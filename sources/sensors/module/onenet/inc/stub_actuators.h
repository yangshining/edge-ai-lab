/**
 * @file    stub_actuators.h
 * @brief   Actuator stubs for onenet demo — replaces beep/fan/led/mq dependencies
 *
 * onenet.c references hardware actuator symbols (Beep_Status, Fan_Set, Led_Set,
 * Mq2_GetPercentage, etc.). This header provides no-op stubs so the module
 * compiles without the actuator drivers.
 */

#ifndef STUB_ACTUATORS_H
#define STUB_ACTUATORS_H

#include "stm32f10x.h"

/* Beep */
#define Beep_Status  0
#define BEEP_ON      1
#define BEEP_OFF     0
#define Beep_Set(x)  do {} while(0)

/* Fan */
#define FAN_ON       1
#define FAN_OFF      0
#define Fan_Set(x)   do {} while(0)

/* LED */
#define LED_ON       1
#define LED_OFF      0
#define Led_Set(x)   do {} while(0)

/* MQ sensors — return 0 (no gas detected) */
static inline uint8_t Mq2_GetPercentage(void) { return 0; }
static inline uint8_t Mq3_GetPercentage(void) { return 0; }
static inline uint8_t Mq4_GetPercentage(void) { return 0; }
static inline uint8_t Mq7_GetPercentage(void) { return 0; }

#endif /* STUB_ACTUATORS_H */
