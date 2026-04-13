#ifndef _MQ_H_
#define _MQ_H_

#include "stm32f10x.h"

/*
 * MQ Series Gas Sensor Library
 * All sensors use DMA-based ADC from adc.c
 * Data is automatically updated in background
 */

// ==================== MQ2 - Smoke/LPG/CO Sensor ====================
uint16_t Mq2_ReadValue(void);      // Get raw ADC value (0-4095)
float Mq2_GetVoltage(void);        // Get voltage (0-3.3V)
uint8_t Mq2_GetPercentage(void);   // Get percentage (0-100)

// ==================== MQ3 - Alcohol Sensor ====================
uint16_t Mq3_ReadValue(void);      // Get raw ADC value (0-4095)
float Mq3_GetVoltage(void);        // Get voltage (0-3.3V)
uint8_t Mq3_GetPercentage(void);   // Get percentage (0-100)

// ==================== MQ4 - Methane/CNG Sensor ====================
uint16_t Mq4_ReadValue(void);      // Get raw ADC value (0-4095)
float Mq4_GetVoltage(void);        // Get voltage (0-3.3V)
uint8_t Mq4_GetPercentage(void);   // Get percentage (0-100)

// ==================== MQ5 - NOT AVAILABLE ====================
// PA2 is USART2_TX (ESP8266). These functions always return 0.
uint16_t Mq5_ReadValue(void);
float Mq5_GetVoltage(void);
uint8_t Mq5_GetPercentage(void);

// ==================== MQ6 - NOT AVAILABLE ====================
// PA3 is USART2_RX (ESP8266). These functions always return 0.
uint16_t Mq6_ReadValue(void);
float Mq6_GetVoltage(void);
uint8_t Mq6_GetPercentage(void);

// ==================== MQ7 - Carbon Monoxide Sensor ====================
uint16_t Mq7_ReadValue(void);      // Get raw ADC value (0-4095)
float Mq7_GetVoltage(void);        // Get voltage (0-3.3V)
uint8_t Mq7_GetPercentage(void);   // Get percentage (0-100)

#endif
