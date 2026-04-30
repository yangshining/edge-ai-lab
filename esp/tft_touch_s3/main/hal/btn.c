#include "btn.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "esp_log.h"

static const char *TAG = "btn";
static button_handle_t s_btn;
static void (*s_click_cb)(void);
static void (*s_long_press_cb)(void);

static void on_click(void *handle, void *usr_data)
{
    (void)handle; (void)usr_data;
    if (s_click_cb) s_click_cb();
}

static void on_long_press(void *handle, void *usr_data)
{
    (void)handle; (void)usr_data;
    if (s_long_press_cb) s_long_press_cb();
}

void btn_init(gpio_num_t gpio)
{
    button_config_t cfg = { .long_press_time = 2000, .short_press_time = 0 };
    button_gpio_config_t gcfg = {
        .gpio_num = gpio,
        .active_level = 0,       /* active-low */
        .enable_power_save = false,
        .disable_pull = false,   /* use internal pull-up */
    };
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&cfg, &gcfg, &s_btn));
    ESP_LOGI(TAG, "button init on GPIO %d", gpio);
}

void btn_set_click_cb(void (*cb)(void))
{
    s_click_cb = cb;
    iot_button_register_cb(s_btn, BUTTON_SINGLE_CLICK, NULL, on_click, NULL);
}

void btn_set_long_press_cb(void (*cb)(void), uint32_t hold_ms)
{
    (void)hold_ms; /* component uses long_press_time set at init */
    s_long_press_cb = cb;
    iot_button_register_cb(s_btn, BUTTON_LONG_PRESS_START, NULL, on_long_press, NULL);
}
