#include "assistant.h"
#include "assistant_state.h"
#include "assistant_proto.h"
#include "audio_service.h"
#include "audio_io.h"
#include "led_status.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "assistant";

#define WS_BUF_SIZE    (1275 + 16 + 32)  /* max audio frame + header + slack */
#define JSON_BUF_SIZE  512

static TaskHandle_t s_task_handle;
static int64_t s_last_attempt_us;

static void on_ws_event(void *handler_arg, esp_event_base_t base,
                        int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    TaskHandle_t task = (TaskHandle_t)handler_arg;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WS connected");
        break;
    case WEBSOCKET_EVENT_DATA:
        (void)data;
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_ERROR:
        xTaskNotify(task, 1, eSetBits);
        break;
    default:
        break;
    }
}

static void assistant_task(void *arg)
{
    (void)arg;
    char json_buf[JSON_BUF_SIZE];
    char session_id[64] = "";
    char text_buf[128]  = "";
    uint8_t audio_buf[AUDIO_OPUS_PKT_MAX_BYTES];
    uint8_t frame_buf[WS_BUF_SIZE];

    /* Connect WebSocket */
    esp_websocket_client_config_t ws_cfg = {
        .uri = CONFIG_ASSISTANT_WS_URL,
        .headers = "Authorization: Bearer " CONFIG_ASSISTANT_WS_TOKEN "\r\n",
        .buffer_size = WS_BUF_SIZE,
        .task_stack = 6 * 1024,
        .task_prio = 5,
    };
    esp_websocket_client_handle_t ws = esp_websocket_client_init(&ws_cfg);
    if (!ws) {
        ESP_LOGE(TAG, "WS init failed");
        goto error;
    }
    esp_websocket_register_events(ws, WEBSOCKET_EVENT_ANY, on_ws_event,
                                  xTaskGetCurrentTaskHandle());

    assistant_state_set_status(ASSISTANT_CONNECTING);
    led_status_set(LED_WORKING);

    if (esp_websocket_client_start(ws) != ESP_OK) {
        ESP_LOGE(TAG, "WS start failed");
        goto error;
    }

    /* Wait for connection (up to 5 s) */
    int wait_ms = 0;
    while (!esp_websocket_client_is_connected(ws) && wait_ms < 5000) {
        vTaskDelay(pdMS_TO_TICKS(100));
        wait_ms += 100;
    }
    if (!esp_websocket_client_is_connected(ws)) {
        ESP_LOGE(TAG, "WS connect timeout");
        goto error;
    }

    /* Send hello */
    proto_make_hello(json_buf, sizeof(json_buf));
    esp_websocket_client_send_text(ws, json_buf, strlen(json_buf), pdMS_TO_TICKS(1000));

    /* Wait for server hello */
    wait_ms = 0;
    bool got_hello = false;
    while (!got_hello && wait_ms < 5000) {
        int len = esp_websocket_client_recv(ws, (char *)frame_buf, sizeof(frame_buf) - 1,
                                            pdMS_TO_TICKS(200));
        wait_ms += 200;
        if (len > 0) {
            frame_buf[len] = '\0';
            server_msg_type_t mt = proto_parse_server_msg(
                (char *)frame_buf, session_id, sizeof(session_id), NULL, 0);
            if (mt == SERVER_MSG_HELLO) {
                got_hello = true;
                assistant_state_set_session(session_id);
                ESP_LOGI(TAG, "server hello, session=%s", session_id);

                /* Parse audio_params for sample_rate + frame_duration */
                cJSON *root = cJSON_Parse((char *)frame_buf);
                if (root) {
                    cJSON *ap = cJSON_GetObjectItem(root, "audio_params");
                    if (ap) {
                        cJSON *sr = cJSON_GetObjectItem(ap, "sample_rate");
                        cJSON *fd = cJSON_GetObjectItem(ap, "frame_duration");
                        uint32_t hz = sr ? (uint32_t)sr->valueint : 24000;
                        int fms     = fd ? fd->valueint : 60;
                        if (audio_service_pause_output() == ESP_OK) {
                            audio_io_set_output_sample_rate(hz);
                            audio_service_set_decode_params(hz, fms);
                            audio_service_resume_output();
                            audio_io_enable_output(true);
                        }
                    }
                    cJSON_Delete(root);
                }
            }
        }
    }
    if (!got_hello) { ESP_LOGE(TAG, "no server hello"); goto error; }

    /* Start listening */
    audio_service_start_input();
    proto_make_listen_start(json_buf, sizeof(json_buf), session_id);
    esp_websocket_client_send_text(ws, json_buf, strlen(json_buf), pdMS_TO_TICKS(1000));
    assistant_state_set_status(ASSISTANT_LISTENING);

    /* Main loop */
    bool listening = true;
    while (1) {
        /* Check for disconnect notification */
        uint32_t notif = 0;
        if (xTaskNotifyWait(0, ULONG_MAX, &notif, 0) == pdTRUE && notif) {
            ESP_LOGW(TAG, "WS disconnected");
            goto error;
        }

        /* Send queued audio frames */
        if (listening) {
            size_t olen = 0; uint32_t ots = 0;
            if (audio_service_pop_send(audio_buf, &olen, &ots)) {
                size_t flen = proto_pack_audio(frame_buf, sizeof(frame_buf),
                                               audio_buf, olen, ots);
                if (flen > 0)
                    esp_websocket_client_send_bin(ws, (char *)frame_buf,
                                                  flen, pdMS_TO_TICKS(200));
                assistant_state_set_status(ASSISTANT_UPLOADING);
            }
        }

        /* Receive from server (non-blocking) */
        int rlen = esp_websocket_client_recv(ws, (char *)frame_buf, sizeof(frame_buf) - 1,
                                             pdMS_TO_TICKS(10));
        if (rlen <= 0) continue;

        /* Binary frame = Opus audio from server */
        if (frame_buf[0] == 0x00) {
            /* Binary Protocol 2: skip 16-byte header */
            if (rlen > 16) {
                audio_service_push_decode(frame_buf + 16, (size_t)(rlen - 16),
                                          xTaskGetTickCount() * portTICK_PERIOD_MS);
            }
            continue;
        }

        /* Text frame = JSON control */
        frame_buf[rlen] = '\0';
        server_msg_type_t mt = proto_parse_server_msg(
            (char *)frame_buf, NULL, 0, text_buf, sizeof(text_buf));

        switch (mt) {
        case SERVER_MSG_STT:
            if (listening) {
                listening = false;
                audio_service_stop_input();
                proto_make_listen_stop(json_buf, sizeof(json_buf), session_id);
                esp_websocket_client_send_text(ws, json_buf, strlen(json_buf), pdMS_TO_TICKS(1000));
                assistant_state_set_status(ASSISTANT_THINKING);
            }
            break;
        case SERVER_MSG_TTS_START:
            assistant_state_set_status(ASSISTANT_SPEAKING);
            led_status_set(LED_SPEAKING);
            if (text_buf[0]) assistant_state_set_reply(text_buf);
            break;
        case SERVER_MSG_TTS_END:
            led_status_set(LED_IDLE);
            audio_io_enable_output(false);
            esp_websocket_client_stop(ws);
            esp_websocket_client_destroy(ws);
            assistant_state_set_status(ASSISTANT_IDLE);
            s_task_handle = NULL;
            vTaskDelete(NULL);
            return;
        default:
            break;
        }
    }

error:
    audio_service_stop_input();
    audio_io_enable_output(false);
    if (ws) {
        esp_websocket_client_stop(ws);
        esp_websocket_client_destroy(ws);
    }
    assistant_state_set_error(ESP_FAIL);
    led_status_set(LED_ERROR);
    s_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t assistant_init(void)
{
    return ESP_OK;
}

void assistant_start_listening(void)
{
    /* Enforce 3 s minimum retry interval */
    int64_t now_us = esp_timer_get_time();
    if (s_last_attempt_us > 0 && (now_us - s_last_attempt_us) < 3000000LL) {
        ESP_LOGW(TAG, "retry too soon, ignoring");
        return;
    }
    if (s_task_handle) {
        ESP_LOGW(TAG, "assistant task already running");
        return;
    }
    s_last_attempt_us = now_us;
    xTaskCreate(assistant_task, "assistant", 8 * 1024, NULL, 5, &s_task_handle);
}
