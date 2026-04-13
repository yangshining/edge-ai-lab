/**
 * @file    demo_main.c
 * @brief   OneNET demo — 完整上传链路验证: WiFi→TCP→MQTT鉴权→发布一条数据
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    ESP8266 TX→PA3(USART2 RX), ESP8266 RX→PA2(USART2 TX)
 * @uart    UART1 PA9/PA10 115200 baud (debug output)
 * @note    上传前必须修改以下配置：
 *          1. src/onenet.c: PROID, ACCESS_KEY, DEVICE_NAME
 *          2. src/esp8266.c: ESP8266_WIFI_INFO (SSID + 密码)
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "esp8266.h"
#include "onenet.h"

#define ESP8266_ONENET_INFO    "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"
#define DEMO_TCP_RETRY_MS      1000   /* wait between TCP CIPSTART retries */
#define DEMO_DEVLINK_RETRY_MS   500   /* wait between MQTT DevLink retries */
#define DEMO_SUBSCRIBE_WAIT_MS  500   /* settle time after Subscribe */
#define DEMO_POLL_WAIT_MS      2000   /* post-publish poll window */

/* Demo sensor values — hardcoded, not from real sensors */
u8  temp      = 25;    /* 温度, °C   */
u8  humi      = 60;    /* 湿度, %    */
int light_pct = 45;    /* 光照, 0-99 */
int mq2_vol   = 0;     /* MQ2, %     */

/* Thresholds (required by onenet.c externs) */
u8  temp_adj = 37;
u8  humi_adj = 90;
int light_th = 30;
int smog_th  = 50;

/* Actuator mode flags (required by onenet.c externs) */
u8  fan_mode = 1;
u8  led_mode = 1;

int main(void)
{
    unsigned char *dataPtr = NULL;
    unsigned char retry    = 0;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[OneNET] Demo data: temp=%d, humi=%d, light=%d\r\n",
                temp, humi, light_pct);

    /* ---- Step 1: WiFi connect ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 1: ESP8266 WiFi init\r\n");
    ESP8266_Init();   /* blocks until WiFi connected */

    /* ---- Step 2: TCP connect to OneNET ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 2: TCP connect to mqtts.heclouds.com:1883\r\n");
    retry = 0;
    while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
    {
        if(++retry > 5)
        {
            UsartPrintf(USART_DEBUG, "[OneNET] ERR: TCP connect failed after 5 retries\r\n");
            while(1) { }
        }
        UsartPrintf(USART_DEBUG, "[OneNET] TCP retry %d...\r\n", retry);
        DelayXms(DEMO_TCP_RETRY_MS);
    }
    UsartPrintf(USART_DEBUG, "[OneNET] TCP connected\r\n");

    /* ---- Step 3: MQTT DevLink (CONNECT + CONNACK) ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 3: MQTT DevLink (HMAC-SHA1 auth)\r\n");
    retry = 0;
    while(OneNet_DevLink())
    {
        if(++retry > 5)
        {
            UsartPrintf(USART_DEBUG, "[OneNET] ERR: DevLink failed after 5 retries\r\n");
            while(1) { }
        }
        ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT");
        DelayXms(DEMO_DEVLINK_RETRY_MS);
    }
    UsartPrintf(USART_DEBUG, "[OneNET] MQTT connected (CONNACK = 0)\r\n");

    /* ---- Step 4: Subscribe ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 4: Subscribe to property/set\r\n");
    OneNET_Subscribe();
    DelayXms(DEMO_SUBSCRIBE_WAIT_MS);
    dataPtr = ESP8266_GetIPD(0);
    if(dataPtr != NULL) OneNet_RevPro(dataPtr);

    /* ---- Step 5: Publish sensor data ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 5: Publish demo data\r\n");
    OneNet_SendData();
    UsartPrintf(USART_DEBUG, "[OneNET] Publish sent\r\n");

    /* ---- Step 6: Poll for cloud response ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 6: Poll cloud response (2s)\r\n");
    DelayXms(DEMO_POLL_WAIT_MS);
    dataPtr = ESP8266_GetIPD(0);
    if(dataPtr != NULL)
    {
        UsartPrintf(USART_DEBUG, "[OneNET] Cloud response received\r\n");
        OneNet_RevPro(dataPtr);
    }
    else
    {
        UsartPrintf(USART_DEBUG, "[OneNET] No cloud response (normal for QoS 0)\r\n");
    }

    /* ---- Halt ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Demo complete\r\n");
    ESP8266_Clear();
    while(1) { }
}
