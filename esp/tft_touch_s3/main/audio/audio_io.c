#include "audio_io.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "audio_io";

static i2s_chan_handle_t s_rx;
static i2s_chan_handle_t s_tx;
static uint32_t s_tx_sample_rate = 24000;

esp_err_t audio_io_init(void)
{
    /* ---- RX: INMP441 on I2S_NUM_0 ---- */
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    rx_chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&rx_chan_cfg, NULL, &s_rx), TAG, "rx new channel");

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_ASSISTANT_MIC_BCLK,
            .ws   = CONFIG_ASSISTANT_MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = CONFIG_ASSISTANT_MIC_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx, &rx_std_cfg), TAG, "rx init std");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_rx), TAG, "rx enable");

    /* ---- TX: MAX98357A on I2S_NUM_1 ---- */
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    tx_chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&tx_chan_cfg, &s_tx, NULL), TAG, "tx new channel");

    i2s_std_config_t tx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(24000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_ASSISTANT_SPK_BCLK,
            .ws   = CONFIG_ASSISTANT_SPK_LRCLK,
            .dout = CONFIG_ASSISTANT_SPK_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &tx_std_cfg), TAG, "tx init std");
    /* TX enabled only when output is unmuted */

    /* SD_MODE pin — start muted */
    gpio_config_t sd_cfg = {
        .pin_bit_mask = BIT64(CONFIG_ASSISTANT_SPK_SD_MODE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&sd_cfg), TAG, "sd_mode gpio config");
    gpio_set_level(CONFIG_ASSISTANT_SPK_SD_MODE, 0);

    ESP_LOGI(TAG, "audio I/O init OK");
    return ESP_OK;
}

int audio_io_read(int16_t *buf, int stereo_pairs)
{
    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(s_rx, buf,
                                     (size_t)stereo_pairs * 2 * sizeof(int16_t),
                                     &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) return -1;
    return (int)(bytes_read / (2 * sizeof(int16_t)));
}

void audio_io_write(const int16_t *buf, int samples)
{
    size_t written;
    i2s_channel_write(s_tx, buf, (size_t)samples * sizeof(int16_t), &written, portMAX_DELAY);
}

esp_err_t audio_io_set_output_sample_rate(uint32_t hz)
{
    if (hz == s_tx_sample_rate) return ESP_OK;
    ESP_RETURN_ON_ERROR(i2s_channel_disable(s_tx), TAG, "tx disable for rate change");
    i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(hz);
    ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_clock(s_tx, &clk), TAG, "tx reconfig clock");
    s_tx_sample_rate = hz;
    ESP_LOGI(TAG, "TX sample rate -> %lu Hz", (unsigned long)hz);
    return ESP_OK;
}

void audio_io_enable_output(bool en)
{
    esp_err_t ret = en ? i2s_channel_enable(s_tx) : i2s_channel_disable(s_tx);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "i2s_channel_%s failed: %s",
                 en ? "enable" : "disable", esp_err_to_name(ret));
        return;
    }
    gpio_set_level(CONFIG_ASSISTANT_SPK_SD_MODE, en ? 1 : 0);
}
