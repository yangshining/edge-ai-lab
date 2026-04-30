#include "audio_service.h"
#include "audio_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_dec_default.h"
#include "esp_opus_enc.h"
#include "esp_opus_dec.h"
#include <string.h>

static const char *TAG = "audio_service";

/* ---- Queue handles ---- */
static QueueHandle_t s_encode_q;    /* audio_pcm_frame_t* (2 slots) */
static QueueHandle_t s_send_q;      /* audio_opus_pkt_t* (40 slots) */
static QueueHandle_t s_decode_q;    /* audio_opus_pkt_t* (40 slots) */
static QueueHandle_t s_playback_q;  /* audio_pcm_frame_t* (2 slots) */

/* ---- Task handles ---- */
static TaskHandle_t s_input_task_handle;
static TaskHandle_t s_codec_task_handle;
static TaskHandle_t s_output_task_handle;

/* ---- Control flags ---- */
static volatile bool s_input_active;
static volatile bool s_output_paused;
static SemaphoreHandle_t s_output_pause_ack; /* given by output task when it pauses */
static SemaphoreHandle_t s_output_resume;    /* given by resume call to unblock output task */

/* ---- Opus handles ---- */
static void *s_enc_handle;
static void *s_dec_handle;
static uint32_t s_dec_sample_rate = 24000;
static int      s_dec_frame_ms    = 60;

/* ---- Encoder frame size: 16 kHz * 60 ms = 960 samples ---- */
#define ENC_FRAME_SAMPLES 960
#define ENC_SAMPLE_RATE   16000

static void open_encoder(void)
{
    esp_opus_enc_config_t cfg = {
        .sample_rate     = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel         = ESP_AUDIO_MONO,
        .bits_per_sample = ESP_AUDIO_BIT16,
        .bitrate         = ESP_OPUS_BITRATE_AUTO,
        .frame_duration  = ESP_OPUS_ENC_FRAME_DURATION_60_MS,
        .application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO,
        .complexity      = 0,
        .enable_fec      = false,
        .enable_dtx      = true,
        .enable_vbr      = true,
    };
    if (esp_opus_enc_open(&cfg, sizeof(cfg), &s_enc_handle) != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "opus enc open failed");
        s_enc_handle = NULL;
    }
}

static void open_decoder(uint32_t sample_rate, int frame_ms)
{
    esp_opus_dec_frame_duration_t fd;
    switch (frame_ms) {
    case 20: fd = ESP_OPUS_DEC_FRAME_DURATION_20_MS; break;
    case 40: fd = ESP_OPUS_DEC_FRAME_DURATION_40_MS; break;
    default: fd = ESP_OPUS_DEC_FRAME_DURATION_60_MS; break;
    }
    esp_opus_dec_cfg_t cfg = {
        .sample_rate    = (uint32_t)sample_rate,
        .channel        = ESP_AUDIO_MONO,
        .frame_duration = fd,
        .self_delimited = false,
    };
    if (esp_opus_dec_open(&cfg, sizeof(cfg), &s_dec_handle) != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "opus dec open failed");
        s_dec_handle = NULL;
    }
}

/* ---- audio_input_task (P5, 6KB) ---- */
static void audio_input_task(void *arg)
{
    (void)arg;
    /* stereo read buffer: 960 stereo pairs = 1920 int16 */
    int16_t *stereo = heap_caps_malloc(ENC_FRAME_SAMPLES * 2 * sizeof(int16_t), MALLOC_CAP_DEFAULT);
    assert(stereo);

    while (1) {
        int got = audio_io_read(stereo, ENC_FRAME_SAMPLES);
        if (got <= 0 || !s_input_active) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        audio_pcm_frame_t *frame = heap_caps_malloc(
            sizeof(audio_pcm_frame_t) + ENC_FRAME_SAMPLES * sizeof(int16_t),
            MALLOC_CAP_DEFAULT);
        if (!frame) { ESP_LOGW(TAG, "input malloc fail"); continue; }

        frame->timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        frame->n_samples    = ENC_FRAME_SAMPLES;
        for (int i = 0; i < ENC_FRAME_SAMPLES; i++) {
            frame->samples[i] = stereo[i * 2]; /* left channel */
        }

        if (xQueueSend(s_encode_q, &frame, 0) != pdTRUE) {
            heap_caps_free(frame); /* queue full, drop */
        }
    }
}

/* ---- opus_codec_task (P3, 24KB) ---- */
static void opus_codec_task(void *arg)
{
    (void)arg;
    uint8_t enc_out[AUDIO_OPUS_PKT_MAX_BYTES];

    while (1) {
        /* --- encode path --- */
        audio_pcm_frame_t *in_frame = NULL;
        if (xQueueReceive(s_encode_q, &in_frame, pdMS_TO_TICKS(5)) == pdTRUE) {
            if (s_enc_handle && in_frame->n_samples == ENC_FRAME_SAMPLES) {
                esp_audio_enc_in_frame_t in = {
                    .buffer = (uint8_t *)in_frame->samples,
                    .len    = ENC_FRAME_SAMPLES * sizeof(int16_t),
                };
                esp_audio_enc_out_frame_t out = {
                    .buffer = enc_out,
                    .len    = sizeof(enc_out),
                    .encoded_bytes = 0,
                };
                if (esp_opus_enc_process(s_enc_handle, &in, &out) == ESP_AUDIO_ERR_OK
                    && out.encoded_bytes > 0) {
                    audio_opus_pkt_t *pkt = heap_caps_malloc(
                        sizeof(audio_opus_pkt_t) + out.encoded_bytes, MALLOC_CAP_DEFAULT);
                    if (pkt) {
                        pkt->timestamp_ms = in_frame->timestamp_ms;
                        pkt->len          = out.encoded_bytes;
                        memcpy(pkt->data, enc_out, out.encoded_bytes);
                        if (xQueueSend(s_send_q, &pkt, 0) != pdTRUE) {
                            heap_caps_free(pkt);
                        }
                    }
                }
            }
            heap_caps_free(in_frame);
        }

        /* --- decode path --- */
        audio_opus_pkt_t *pkt = NULL;
        if (xQueueReceive(s_decode_q, &pkt, 0) == pdTRUE) {
            if (s_dec_handle) {
                size_t n_out = (size_t)(s_dec_sample_rate * s_dec_frame_ms / 1000);
                audio_pcm_frame_t *out_frame = heap_caps_malloc(
                    sizeof(audio_pcm_frame_t) + n_out * sizeof(int16_t), MALLOC_CAP_DEFAULT);
                if (out_frame) {
                    out_frame->n_samples = n_out;
                    esp_audio_dec_in_raw_t raw = {
                        .buffer        = pkt->data,
                        .len           = (uint32_t)pkt->len,
                        .consumed      = 0,
                        .frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE,
                    };
                    esp_audio_dec_out_frame_t of = {
                        .buffer       = (uint8_t *)out_frame->samples,
                        .len          = (uint32_t)(n_out * sizeof(int16_t)),
                        .decoded_size = 0,
                    };
                    esp_audio_dec_info_t info = {};
                    if (esp_opus_dec_decode(s_dec_handle, &raw, &of, &info) == ESP_AUDIO_ERR_OK) {
                        out_frame->timestamp_ms = pkt->timestamp_ms;
                        out_frame->n_samples    = of.decoded_size / sizeof(int16_t);
                        if (xQueueSend(s_playback_q, &out_frame, 0) != pdTRUE) {
                            heap_caps_free(out_frame);
                        }
                    } else {
                        heap_caps_free(out_frame);
                    }
                }
            }
            heap_caps_free(pkt);
        }
    }
}

/* ---- audio_output_task (P4, 4KB) ---- */
static void audio_output_task(void *arg)
{
    (void)arg;
    while (1) {
        if (s_output_paused) {
            xSemaphoreGive(s_output_pause_ack);
            xSemaphoreTake(s_output_resume, portMAX_DELAY);
            continue;
        }
        audio_pcm_frame_t *frame = NULL;
        if (xQueueReceive(s_playback_q, &frame, pdMS_TO_TICKS(20)) == pdTRUE) {
            audio_io_write(frame->samples, (int)frame->n_samples);
            heap_caps_free(frame);
        }
    }
}

/* ---- Public API ---- */

esp_err_t audio_service_init(void)
{
    s_encode_q   = xQueueCreate(2,  sizeof(void *));
    s_send_q     = xQueueCreate(40, sizeof(void *));
    s_decode_q   = xQueueCreate(40, sizeof(void *));
    s_playback_q = xQueueCreate(2,  sizeof(void *));
    s_output_pause_ack = xSemaphoreCreateBinary();
    s_output_resume    = xSemaphoreCreateBinary();

    if (!s_encode_q || !s_send_q || !s_decode_q || !s_playback_q ||
        !s_output_pause_ack || !s_output_resume) {
        ESP_LOGE(TAG, "resource allocation failed");
        return ESP_ERR_NO_MEM;
    }

    open_encoder();
    open_decoder(s_dec_sample_rate, s_dec_frame_ms);

    if (xTaskCreate(audio_input_task, "a_in",  6 * 1024, NULL, 5, &s_input_task_handle) != pdPASS ||
        xTaskCreate(opus_codec_task,  "a_cod", 24 * 1024, NULL, 3, &s_codec_task_handle) != pdPASS ||
        xTaskCreate(audio_output_task,"a_out", 4 * 1024, NULL, 4, &s_output_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "task creation failed");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "audio service init OK");
    return ESP_OK;
}

void audio_service_start_input(void)  { s_input_active = true; }
void audio_service_stop_input(void)   { s_input_active = false; }

void audio_service_push_decode(const uint8_t *opus, size_t len, uint32_t ts_ms)
{
    audio_opus_pkt_t *pkt = heap_caps_malloc(sizeof(audio_opus_pkt_t) + len, MALLOC_CAP_DEFAULT);
    if (!pkt) return;
    pkt->timestamp_ms = ts_ms;
    pkt->len = len;
    memcpy(pkt->data, opus, len);
    if (xQueueSend(s_decode_q, &pkt, 0) != pdTRUE) heap_caps_free(pkt);
}

bool audio_service_pop_send(uint8_t *buf, size_t *len, uint32_t *ts_ms)
{
    audio_opus_pkt_t *pkt = NULL;
    if (xQueueReceive(s_send_q, &pkt, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    *len   = pkt->len;
    *ts_ms = pkt->timestamp_ms;
    memcpy(buf, pkt->data, pkt->len);
    heap_caps_free(pkt);
    return true;
}

esp_err_t audio_service_pause_output(void)
{
    s_output_paused = true;
    if (xSemaphoreTake(s_output_pause_ack, pdMS_TO_TICKS(200)) != pdTRUE) {
        s_output_paused = false;
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void audio_service_resume_output(void)
{
    s_output_paused = false;
    xSemaphoreGive(s_output_resume);
}

void audio_service_set_decode_params(uint32_t sample_rate, int frame_ms)
{
    if (s_dec_handle) {
        esp_opus_dec_close(s_dec_handle);
        s_dec_handle = NULL;
    }
    s_dec_sample_rate = sample_rate;
    s_dec_frame_ms    = frame_ms;
    open_decoder(sample_rate, frame_ms);
    ESP_LOGI(TAG, "decoder params: %lu Hz / %d ms", (unsigned long)sample_rate, frame_ms);
}
