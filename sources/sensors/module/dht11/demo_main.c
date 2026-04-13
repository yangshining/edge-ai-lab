/**
 * @file    demo_main.c
 * @brief   DHT11 demo — 循环读取温湿度并通过 UART1 输出
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    DHT11 DATA → PA0 (注意: 头文件注释写 PA11, 宏实际操作 PA0)
 *          UART1 TX   → PA9  (115200 baud, 调试输出)
 * @note    无需额外配置，上电即运行
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "dht11.h"

#define READ_INTERVAL_MS  1000   /* 读取间隔, ms */

int main(void)
{
    uint8_t temp = 0, humi = 0;
    uint8_t ret  = 0;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[DHT11] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[DHT11] Pin: PA0, Interval: %d ms\r\n", READ_INTERVAL_MS);

    DHT11_Init();
    UsartPrintf(USART_DEBUG, "[DHT11] Init OK\r\n");

    /* ---- Loop ---- */
    while(1)
    {
        ret = DHT11_Read_Data(&temp, &humi);

        if(ret == 0)
        {
            UsartPrintf(USART_DEBUG, "[DHT11] Temp: %d C, Humi: %d %%\r\n", temp, humi);
        }
        else
        {
            UsartPrintf(USART_DEBUG, "[DHT11] Read failed (code %d)\r\n", ret);
        }

        DelayXms(READ_INTERVAL_MS);
    }
}
