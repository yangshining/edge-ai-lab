#pragma once
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_IDLE,
    LED_WORKING,
    LED_SPEAKING,
    LED_ERROR,
} led_status_t;

void led_status_init(gpio_num_t gpio);
void led_status_set(led_status_t status);

#ifdef __cplusplus
}
#endif
