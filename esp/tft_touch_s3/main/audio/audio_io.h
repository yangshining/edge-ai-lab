#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t audio_io_init(void);

/* Reads stereo_pairs stereo pairs from I2S_NUM_0 (INMP441).
 * buf must hold stereo_pairs * 2 int16 values.
 * Returns number of stereo pairs actually read, or -1 on error. */
int audio_io_read(int16_t *buf, int stereo_pairs);

/* Writes samples mono int16 values to I2S_NUM_1 (MAX98357A). */
void audio_io_write(const int16_t *buf, int samples);

/* Disable TX, reconfigure I2S clock, re-enable TX.
 * Caller must call audio_service_pause_output() first. */
esp_err_t audio_io_set_output_sample_rate(uint32_t hz);

/* HIGH = enable amp output; LOW = mute (SD_MODE pin). */
void audio_io_enable_output(bool en);

#ifdef __cplusplus
}
#endif
