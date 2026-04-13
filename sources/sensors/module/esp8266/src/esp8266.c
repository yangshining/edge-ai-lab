/** @file esp8266.c
 * @brief ESP8266 WiFi module AT-command driver (USART2 PA2/PA3)
 */

#include "stm32f10x.h"

#include "esp8266.h"

#include "delay.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"CMCC-4DHK\",\"3LLLL3MV\"\r\n"


volatile unsigned char esp8266_buf[512];
volatile unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;


/* ESP8266_Clear: clear receive buffer */
void ESP8266_Clear(void)
{
	unsigned short i;
	for(i = 0; i < sizeof(esp8266_buf); i++)
		esp8266_buf[i] = 0;
	esp8266_cnt = 0;
}

/* ESP8266_WaitRecive: poll until receive is complete; returns REV_OK or REV_WAIT */
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0)
		return REV_WAIT;

	if(esp8266_cnt == esp8266_cntPre)
	{
		esp8266_cnt = 0;

		return REV_OK;
	}

	esp8266_cntPre = esp8266_cnt;

	return REV_WAIT;

}

/* ESP8266_SendCmd: send AT command and wait for expected response; returns 0 on success */
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	unsigned char timeOut = 200;
	char snap[sizeof(esp8266_buf)];

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));

	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)
		{
			/* Copy volatile buffer to local snapshot before string search */
			memcpy(snap, (const void *)esp8266_buf, sizeof(snap));
			if(strstr(snap, res) != NULL)
			{
				ESP8266_Clear();
				return 0;
			}
		}

		DelayXms(10);
	}

	return 1;
}

/* ESP8266_SendData: send raw data via AT+CIPSEND */
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];

	ESP8266_Clear();
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);
	if(!ESP8266_SendCmd(cmdBuf, ">"))
	{
		Usart_SendString(USART2, data, len);
	}

}

/* ESP8266_GetIPD: wait for +IPD response and return pointer to payload; NULL on timeout */
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;

	do
	{
		if(ESP8266_WaitRecive() == REV_OK)
		{
			ptrIPD = strstr((char *)(void *)esp8266_buf, "IPD,");
			if(ptrIPD == NULL)
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;

			}
		}

		DelayXms(5);
	} while(timeOut--);

	return NULL;

}

/* MODULE CHANGE: OLED calls replaced with UsartPrintf for portability */
void ESP8266_Init(void)
{
	Usart2_Init(115200);

	UsartPrintf(USART_DEBUG, "[ESP8266] Init start\r\n");

	while(ESP8266_SendCmd("AT\r\n", "OK"))
		DelayXms(500);
	UsartPrintf(USART_DEBUG, "[ESP8266] 1/4 AT OK\r\n");

	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		DelayXms(500);
	UsartPrintf(USART_DEBUG, "[ESP8266] 2/4 CWMODE=1 OK\r\n");

	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		DelayXms(500);
	UsartPrintf(USART_DEBUG, "[ESP8266] 3/4 CWDHCP OK\r\n");

	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "OK"))
		DelayXms(500);
	UsartPrintf(USART_DEBUG, "[ESP8266] 4/4 WiFi connected OK\r\n");
}

/* USART2_IRQHandler: USART2 RX interrupt — fill esp8266_buf */
void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0;
		esp8266_buf[esp8266_cnt++] = USART2->DR;

		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}

}
