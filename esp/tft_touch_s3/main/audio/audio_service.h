#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Max Opus packet size per RFC 6716 §3.4 */
#define AUDIO_OPUS_PKT_MAX_BYTES 1275

/* Heap-allocated queue item types */
typedef struct {
    uint32_t timestamp_ms;
    size_t   n_samples;
    int16_t  samples[];   /* flexible array; alloc = sizeof()+n_samples*sizeof(int16_t) */
} audio_pcm_frame_t;

typedef struct {
    uint32_t timestamp_ms;
    size_t   len;
    uint8_t  data[];      /* flexible array; alloc = sizeof()+len */
} audio_opus_pkt_t;

esp_err_t audio_service_init(void);

/* Control mic capture → encode_queue */
void audio_service_start_input(void);
void audio_service_stop_input(void);

/* Push an Opus packet from the network into decode_queue (called by assistant_task) */
void audio_service_push_decode(const uint8_t *opus, size_t len, uint32_t timestamp_ms);

/* Blocking 100 ms dequeue from send_queue; copies payload into buf (max AUDIO_OPUS_PKT_MAX_BYTES).
 * Returns true on success, false on timeout. */
bool audio_service_pop_send(uint8_t *buf, size_t *len, uint32_t *timestamp_ms);

/* Pause output path (drain + block audio_output_task). Timeout 200 ms.
 * Returns ESP_ERR_TIMEOUT if output task is stuck. */
esp_err_t audio_service_pause_output(void);
void      audio_service_resume_output(void);

/* Must be called while output is paused. Closes old Opus decoder, opens new one. */
void audio_service_set_decode_params(uint32_t sample_rate, int frame_ms);

void audio_service_flush_input_path(void);
void audio_service_flush_output_path(void);
void audio_service_flush_all(void);
bool audio_service_wait_output_drain(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
