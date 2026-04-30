#include "led_status.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "led_status";

static led_strip_handle_t s_strip;
static led_status_t s_status = LED_IDLE;
static esp_timer_handle_t s_blink_timer;
static bool s_blink_on;
static SemaphoreHandle_t s_lock;

static void apply_color(uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(s_strip, 0, r, g, b);
    led_strip_refresh(s_strip);
}

static void blink_timer_cb(void *arg)
{
    (void)arg;
    if (xSemaphoreTake(s_lock, 0) != pdTRUE) return;
    s_blink_on = !s_blink_on;
    switch (s_status) {
    case LED_WORKING:
        apply_color(0, s_blink_on ? 32 : 0, 0);
        break;
    case LED_ERROR:
        apply_color(s_blink_on ? 32 : 0, 0, 0);
        break;
    default:
        break;
    }
    xSemaphoreGive(s_lock);
}

void led_status_init(gpio_num_t gpio)
{
    s_lock = xSemaphoreCreateMutex();

    led_strip_config_t strip_cfg = {
        .strip_gpio_num = gpio,
        .max_leds = 1,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .resolution_hz = 10 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip));
    led_strip_clear(s_strip);

    const esp_timer_create_args_t timer_args = {
        .callback = blink_timer_cb,
        .name = "led_blink",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_blink_timer));
    ESP_LOGI(TAG, "LED init on GPIO %d", gpio);
}

void led_status_set(led_status_t status)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_status = status;
    esp_timer_stop(s_blink_timer);
    s_blink_on = true;

    switch (status) {
    case LED_IDLE:
        apply_color(0, 0, 32);          /* blue steady */
        break;
    case LED_WORKING:
        apply_color(0, 32, 0);          /* green, breathe via 500ms blink */
        esp_timer_start_periodic(s_blink_timer, 500 * 1000);
        break;
    case LED_SPEAKING:
        apply_color(0, 20, 0);          /* green steady (dim) */
        break;
    case LED_ERROR:
        apply_color(32, 0, 0);          /* red fast blink */
        esp_timer_start_periodic(s_blink_timer, 200 * 1000);
        break;
    }
    xSemaphoreGive(s_lock);
}
