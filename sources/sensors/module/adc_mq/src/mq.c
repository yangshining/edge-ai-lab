#include "stm32f10x.h"
#include "adc.h"
#include "mq.h"

/*
 * MQ Series Gas Sensor Driver
 *
 * All raw values come from the DMA-backed ADC_ConvertedValue[] array.
 * Channel-to-index mapping is defined by ADC_IDX_* in adc.h.
 *
 * NOTE: MQ5 (PA2) and MQ6 (PA3) are NOT available on this board because
 * PA2/PA3 are used by USART2 for ESP8266 communication.
 */

#define ADC_TO_VOLTAGE(adc)     (3.3f / 4095.0f * (adc))

static uint8_t adc_to_pct(uint16_t v)
{
	uint32_t p = (uint32_t)v * 100u / 4095u;
	return (uint8_t)(p > 100u ? 100u : p);
}

// ==================== MQ2 — Smoke / LPG / CO ====================
uint16_t Mq2_ReadValue(void)    { return ADC_ConvertedValue[ADC_IDX_MQ2]; }
float    Mq2_GetVoltage(void)   { return ADC_TO_VOLTAGE(Mq2_ReadValue()); }
uint8_t  Mq2_GetPercentage(void){ return adc_to_pct(Mq2_ReadValue()); }

// ==================== MQ3 — Alcohol ====================
uint16_t Mq3_ReadValue(void)    { return ADC_ConvertedValue[ADC_IDX_MQ3]; }
float    Mq3_GetVoltage(void)   { return ADC_TO_VOLTAGE(Mq3_ReadValue()); }
uint8_t  Mq3_GetPercentage(void){ return adc_to_pct(Mq3_ReadValue()); }

// ==================== MQ4 — Methane / CNG ====================
uint16_t Mq4_ReadValue(void)    { return ADC_ConvertedValue[ADC_IDX_MQ4]; }
float    Mq4_GetVoltage(void)   { return ADC_TO_VOLTAGE(Mq4_ReadValue()); }
uint8_t  Mq4_GetPercentage(void){ return adc_to_pct(Mq4_ReadValue()); }

// ==================== MQ5 — NOT AVAILABLE (PA2 = USART2_TX) ====================
uint16_t Mq5_ReadValue(void)    { return 0; }
float    Mq5_GetVoltage(void)   { return 0.0f; }
uint8_t  Mq5_GetPercentage(void){ return 0; }

// ==================== MQ6 — NOT AVAILABLE (PA3 = USART2_RX) ====================
uint16_t Mq6_ReadValue(void)    { return 0; }
float    Mq6_GetVoltage(void)   { return 0.0f; }
uint8_t  Mq6_GetPercentage(void){ return 0; }

// ==================== MQ7 — Carbon Monoxide ====================
uint16_t Mq7_ReadValue(void)    { return ADC_ConvertedValue[ADC_IDX_MQ7]; }
float    Mq7_GetVoltage(void)   { return ADC_TO_VOLTAGE(Mq7_ReadValue()); }
uint8_t  Mq7_GetPercentage(void){ return adc_to_pct(Mq7_ReadValue()); }
