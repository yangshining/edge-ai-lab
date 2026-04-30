#include "assistant.h"
#include "assistant_state.h"
#include "assistant_proto.h"
#include "audio_service.h"
#include "audio_io.h"
#include "led_status.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_websocket_client.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "assistant";

#define WS_BUF_SIZE    (1275 + 32)  /* max raw Opus frame + slack */
#define JSON_BUF_SIZE  512
#define WS_HEADERS_SIZE 256

static TaskHandle_t s_task_handle;
static int64_t s_last_attempt_us;

typedef struct {
    bool is_binary;
    size_t len;
    uint8_t data[];
} ws_msg_t;

typedef struct {
    TaskHandle_t task;
    QueueHandle_t queue;
} ws_event_ctx_t;

static void free_ws_queue(QueueHandle_t q)
{
    ws_msg_t *msg = NULL;
    while (q && xQueueReceive(q, &msg, 0) == pdTRUE) {
        heap_caps_free(msg);
    }
}

static void make_device_id(char *buf, size_t size)
{
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(buf, size, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void make_uuid(char *buf, size_t size)
{
    uint32_t a = esp_random();
    uint32_t b = esp_random();
    uint32_t c = esp_random();
    uint32_t d = esp_random();
    snprintf(buf, size, "%08lx-%04lx-%04lx-%04lx-%04lx%08lx",
             (unsigned long)a,
             (unsigned long)(b >> 16),
             (unsigned long)(b & 0xffff),
             (unsigned long)(c >> 16),
             (unsigned long)(c & 0xffff),
             (unsigned long)d);
}

static void get_client_id(char *buf, size_t size)
{
    nvs_handle_t h;
    if (nvs_open("assistant", NVS_READWRITE, &h) != ESP_OK) {
        make_uuid(buf, size);
        return;
    }
    size_t required = size;
    esp_err_t err = nvs_get_str(h, "client_id", buf, &required);
    if (err != ESP_OK || buf[0] == '\0') {
        make_uuid(buf, size);
        nvs_set_str(h, "client_id", buf);
        nvs_commit(h);
    }
    nvs_close(h);
}

static void make_ws_headers(char *buf, size_t size)
{
    char device_id[18] = "";
    char client_id[40] = "";
    make_device_id(device_id, sizeof(device_id));
    get_client_id(client_id, sizeof(client_id));

    if (CONFIG_ASSISTANT_WS_TOKEN[0] != '\0') {
        snprintf(buf, size,
                 "Authorization: Bearer " CONFIG_ASSISTANT_WS_TOKEN "\r\n"
                 "Protocol-Version: 1\r\n"
                 "Device-Id: %s\r\n"
                 "Client-Id: %s\r\n",
                 device_id, client_id);
    } else {
        snprintf(buf, size,
                 "Protocol-Version: 1\r\n"
                 "Device-Id: %s\r\n"
                 "Client-Id: %s\r\n",
                 device_id, client_id);
    }
}

static void on_ws_event(void *handler_arg, esp_event_base_t base,
                        int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    ws_event_ctx_t *ctx = (ws_event_ctx_t *)handler_arg;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WS connected");
        break;
    case WEBSOCKET_EVENT_DATA: {
        if (!data || data->data_len <= 0 || !ctx || !ctx->queue) {
            break;
        }
        ws_msg_t *msg = heap_caps_malloc(sizeof(ws_msg_t) + data->data_len + 1,
                                         MALLOC_CAP_DEFAULT);
        if (!msg) {
            ESP_LOGW(TAG, "drop WS frame: no memory");
            break;
        }
        msg->is_binary = data->op_code == WS_TRANSPORT_OPCODES_BINARY;
        msg->len = (size_t)data->data_len;
        memcpy(msg->data, data->data_ptr, msg->len);
        msg->data[msg->len] = '\0';
        if (xQueueSend(ctx->queue, &msg, 0) != pdTRUE) {
            heap_caps_free(msg);
            ESP_LOGW(TAG, "drop WS frame: queue full");
        }
        break;
    }
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_ERROR:
        if (ctx && ctx->task) {
            xTaskNotify(ctx->task, 1, eSetBits);
        }
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
    char headers[WS_HEADERS_SIZE];
    esp_websocket_client_handle_t ws = NULL;
    QueueHandle_t ws_queue = xQueueCreate(8, sizeof(ws_msg_t *));
    ws_event_ctx_t ws_ctx = {
        .task = xTaskGetCurrentTaskHandle(),
        .queue = ws_queue,
    };

    if (!ws_queue) {
        ESP_LOGE(TAG, "WS queue allocation failed");
        goto error;
    }

    make_ws_headers(headers, sizeof(headers));

    /* Connect WebSocket */
    esp_websocket_client_config_t ws_cfg = {
        .uri = CONFIG_ASSISTANT_WS_URL,
        .headers = headers,
        .buffer_size = WS_BUF_SIZE,
        .task_stack = 6 * 1024,
        .task_prio = 5,
    };
    ws = esp_websocket_client_init(&ws_cfg);
    if (!ws) {
        ESP_LOGE(TAG, "WS init failed");
        goto error;
    }
    esp_websocket_register_events(ws, WEBSOCKET_EVENT_ANY, on_ws_event,
                                  &ws_ctx);

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
        ws_msg_t *msg = NULL;
        bool has_msg = xQueueReceive(ws_queue, &msg, pdMS_TO_TICKS(200)) == pdTRUE;
        wait_ms += 200;
        if (has_msg && msg) {
            if (msg->is_binary) {
                heap_caps_free(msg);
                continue;
            }
            server_msg_type_t mt = proto_parse_server_msg(
                (char *)msg->data, session_id, sizeof(session_id), NULL, 0);
            if (mt == SERVER_MSG_HELLO) {
                got_hello = true;
                assistant_state_set_session(session_id);
                ESP_LOGI(TAG, "server hello, session=%s", session_id);

                /* Parse audio_params for sample_rate + frame_duration */
                cJSON *root = cJSON_Parse((char *)msg->data);
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
            heap_caps_free(msg);
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
                esp_websocket_client_send_bin(ws, (char *)audio_buf,
                                              (int)olen, pdMS_TO_TICKS(200));
                assistant_state_set_status(ASSISTANT_UPLOADING);
            }
        }

        ws_msg_t *msg = NULL;
        if (xQueueReceive(ws_queue, &msg, pdMS_TO_TICKS(10)) != pdTRUE) {
            continue;
        }

        if (msg->is_binary) {
            audio_service_push_decode(msg->data, msg->len,
                                      xTaskGetTickCount() * portTICK_PERIOD_MS);
            heap_caps_free(msg);
            continue;
        }

        /* Text frame = JSON control */
        server_msg_type_t mt = proto_parse_server_msg(
            (char *)msg->data, NULL, 0, text_buf, sizeof(text_buf));
        heap_caps_free(msg);

        switch (mt) {
        case SERVER_MSG_STT:
            if (listening) {
                listening = false;
                audio_service_stop_input();
                audio_service_flush_input_path();
                proto_make_listen_stop(json_buf, sizeof(json_buf), session_id);
                esp_websocket_client_send_text(ws, json_buf, strlen(json_buf), pdMS_TO_TICKS(1000));
                assistant_state_set_status(ASSISTANT_THINKING);
            }
            break;
        case SERVER_MSG_TTS_START:
            audio_service_stop_input();
            audio_service_flush_input_path();
            assistant_state_set_status(ASSISTANT_SPEAKING);
            led_status_set(LED_SPEAKING);
            if (text_buf[0]) assistant_state_set_reply(text_buf);
            break;
        case SERVER_MSG_TTS_END:
            if (!audio_service_wait_output_drain(500)) {
                audio_service_flush_output_path();
            }
            led_status_set(LED_IDLE);
            audio_io_enable_output(false);
            esp_websocket_client_stop(ws);
            esp_websocket_client_destroy(ws);
            free_ws_queue(ws_queue);
            vQueueDelete(ws_queue);
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
    audio_service_flush_all();
    audio_io_enable_output(false);
    if (ws) {
        esp_websocket_client_stop(ws);
        esp_websocket_client_destroy(ws);
    }
    free_ws_queue(ws_queue);
    if (ws_queue) {
        vQueueDelete(ws_queue);
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
