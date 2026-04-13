/**
 * @file    demo_main.c
 * @brief   DHT11 demo: UART1 and OLED show temperature/humidity
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    DHT11 DATA -> PA0
 *          OLED SCL  -> PB1
 *          OLED SDA  -> PB0
 *          UART1 TX  -> PA9
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "dht11.h"
#include "oled.h"

#define READ_INTERVAL_MS  1000   /* 读取间隔, ms */

int main(void)
{
    uint8_t temp = 0, humi = 0;
    uint8_t ret  = 0;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);
    OLED_Init();
    OLED_Clear();

    UsartPrintf(USART_DEBUG, "\r\n[DHT11] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[DHT11] Pin: PA0, Interval: %d ms\r\n", READ_INTERVAL_MS);

    DHT11_Init();
    UsartPrintf(USART_DEBUG, "[DHT11] Init OK\r\n");

    OLED_ShowString(0, 0, (u8 *)"DHT11 Demo", 16);
    OLED_ShowString(0, 2, (u8 *)"Temp:     C", 16);
    OLED_ShowString(0, 4, (u8 *)"Humi:     %", 16);
    OLED_ShowString(0, 6, (u8 *)"Status: Init", 16);

    /* ---- Loop ---- */
    while(1)
    {
        ret = DHT11_Read_Data(&temp, &humi);

        if(ret == 0)
        {
            UsartPrintf(USART_DEBUG, "[DHT11] Temp: %d C, Humi: %d %%\r\n", temp, humi);
            OLED_ShowNum(48, 2, temp, 3, 16);
            OLED_ShowNum(48, 4, humi, 3, 16);
            OLED_ShowString(0, 6, (u8 *)"Status: OK  ", 16);
        }
        else
        {
            UsartPrintf(USART_DEBUG, "[DHT11] Read failed (code %d)\r\n", ret);
            OLED_ShowString(48, 2, (u8 *)"---", 16);
            OLED_ShowString(48, 4, (u8 *)"---", 16);
            if(ret == 1)
                OLED_ShowString(0, 6, (u8 *)"Status: NoAck", 16);
            else
                OLED_ShowString(0, 6, (u8 *)"Status: CRC ", 16);
        }

        DelayXms(READ_INTERVAL_MS);
    }
}
