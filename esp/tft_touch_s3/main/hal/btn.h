#pragma once
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void btn_init(gpio_num_t gpio);
void btn_set_click_cb(void (*cb)(void));
void btn_set_long_press_cb(void (*cb)(void), uint32_t hold_ms);

#ifdef __cplusplus
}
#endif
