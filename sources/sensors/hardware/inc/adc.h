#ifndef __ADC_H
#define	__ADC_H


#include "stm32f10x.h"

/********************ADC1 through DMA1 channel 1, ADC3 through DMA2 channel 5, ADC2 does not have DMA**************************/
#define    ADC_APBxClock_FUN             RCC_APB2PeriphClockCmd
#define    ADC_CLK                       RCC_APB2Periph_ADC1

#define    ADC_GPIO_APBxClock_FUN        RCC_APB2PeriphClockCmd

#define    ADC_GPIO_CLK                  RCC_APB2Periph_GPIOA
#define    ADC_PORT                      GPIOA

/*
 * ADC channel count: 5 channels in use
 *
 * NOTE: PA2 and PA3 are reserved for USART2 (ESP8266 TX/RX) and MUST NOT
 *       be configured as ADC inputs. MQ5 (PA2) and MQ6 (PA3) are therefore
 *       not available on this hardware configuration.
 *
 * Conversion order (rank) and DMA buffer index mapping:
 *   rank 1  ->  [0]  PA0  ADC_Channel_0  MQ3  (Alcohol)
 *   rank 2  ->  [1]  PA1  ADC_Channel_1  MQ4  (Methane)
 *   rank 3  ->  [2]  PA4  ADC_Channel_4  Light sensor
 *   rank 4  ->  [3]  PA5  ADC_Channel_5  MQ2  (Smoke/CO)
 *   rank 5  ->  [4]  PA6  ADC_Channel_6  MQ7  (Carbon Monoxide)
 */
#define    NOFCHANEL                     5

/* DMA buffer index aliases — use these instead of raw numbers */
#define    ADC_IDX_MQ3                   0   /* PA0, Alcohol */
#define    ADC_IDX_MQ4                   1   /* PA1, Methane */
#define    ADC_IDX_LIGHT                 2   /* PA4, Light sensor */
#define    ADC_IDX_MQ2                   3   /* PA5, Smoke/CO */
#define    ADC_IDX_MQ7                   4   /* PA6, Carbon Monoxide */

// ADC1 corresponds to DMA1 channel 1
#define    ADC_x                         ADC1
#define    ADC_DMA_CHANNEL               DMA1_Channel1
#define    ADC_DMA_CLK                   RCC_AHBPeriph_DMA1


extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];

/**************************ADC Initialization********************************/
void               ADCx_Init                               (void);


#endif /* __ADC_H */
