/**
 * @file    demo_main.c
 * @brief   ESP8266 demo — WiFi 连接并打印 IP 地址
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    ESP8266 TX→PA3(USART2 RX), ESP8266 RX→PA2(USART2 TX)
 * @uart    UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    修改 src/esp8266.c 中 ESP8266_WIFI_INFO 宏为实际 WiFi SSID 和密码
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "esp8266.h"

#define CIFSR_CMD   "AT+CIFSR\r\n"   /* 查询本机 IP 地址 */

int main(void)
{
    unsigned char *resp = NULL;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[ESP8266] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[ESP8266] USART2: PA2(TX) PA3(RX) 115200 baud\r\n");

    /* ---- Run ---- */
    ESP8266_Init();   /* blocks until WiFi connected */

    /* Query and print IP address */
    UsartPrintf(USART_DEBUG, "[ESP8266] Querying IP (AT+CIFSR)...\r\n");
    ESP8266_SendCmd(CIFSR_CMD, "STAIP");
    DelayXms(200);
    resp = ESP8266_GetIPD(0);
    if(resp != NULL)
        UsartPrintf(USART_DEBUG, "[ESP8266] Response: %s\r\n", resp);
    else
        UsartPrintf(USART_DEBUG, "[ESP8266] No response to AT+CIFSR\r\n");

    ESP8266_Clear();

    /* ---- Halt ---- */
    UsartPrintf(USART_DEBUG, "[ESP8266] Demo complete\r\n");
    while(1) { }
}
