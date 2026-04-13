/**
 * @file    demo_main.c
 * @brief   OLED demo — 显示静态标题 + 自增计数器，UART1 同步输出
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    OLED SCL→PB1, OLED SDA→PB0 (软件 I2C，bit-bang)
 * @uart    UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    OLED 使用软件 I2C，无需额外硬件配置
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"

#define REFRESH_INTERVAL_MS  500    /* 刷新间隔, ms */
#define COUNT_MAX            9999   /* 计数上限，循环归零 */

int main(void)
{
    unsigned short count = 0;
    char buf[8];

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[OLED] Demo start\r\n");

    OLED_Init();
    OLED_Clear();

    /* 静态内容 */
    OLED_ShowString(0, 0, "STM32 OLED Demo", 16);
    OLED_ShowString(0, 2, "Hello World", 16);
    UsartPrintf(USART_DEBUG, "[OLED] Static content displayed\r\n");

    /* ---- Loop ---- */
    while(1)
    {
        snprintf(buf, sizeof(buf), "%4d", count);
        OLED_ShowString(0, 4, "Count:", 16);
        OLED_ShowString(64, 4, buf, 16);

        UsartPrintf(USART_DEBUG, "[OLED] Count: %d\r\n", count);

        count = (count >= COUNT_MAX) ? 0 : count + 1;

        DelayXms(REFRESH_INTERVAL_MS);
    }
}
