/**
 * @file    demo_main.c
 * @brief   ADC + MQ 传感器 demo — DMA 5 通道采集，循环打印原始值和百分比
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    MQ3→PA0, MQ4→PA1, LIGHT→PA4, MQ2→PA5, MQ7→PA6 (ADC 模拟输入)
 *          UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    PA2/PA3 已被 USART2 占用，MQ5/MQ6 无法接入
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "adc.h"
#include "mq.h"

#define PRINT_INTERVAL_MS  1000   /* 打印间隔, ms */

int main(void)
{
    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[ADC_MQ] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[ADC_MQ] 5-channel DMA ADC: MQ3(PA0) MQ4(PA1) LIGHT(PA4) MQ2(PA5) MQ7(PA6)\r\n");

    ADCx_Init();
    DelayXms(100);   /* 等待 DMA 填充首批数据 */
    UsartPrintf(USART_DEBUG, "[ADC_MQ] ADC+DMA init OK\r\n\r\n");

    /* ---- Loop ---- */
    while(1)
    {
        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ3 (PA0) Alcohol  : raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ3],  Mq3_GetPercentage());

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ4 (PA1) Methane  : raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ4],  Mq4_GetPercentage());

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] LIGHT(PA4) Luminance: raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_LIGHT], (int)(ADC_ConvertedValue[ADC_IDX_LIGHT] / 4095.0f * 100));

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ2 (PA5) Smoke    : raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ2],  Mq2_GetPercentage());

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ7 (PA6) CO       : raw=%4d  %2d%%\r\n\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ7],  Mq7_GetPercentage());

        DelayXms(PRINT_INTERVAL_MS);
    }
}
