#include "stm32f10x.h"

#include "onenet.h"
#include "esp8266.h"
#include "delay.h"
#include "usart.h"
#include "beep.h"
#include "fan.h"
#include "dht11.h"
#include "oled.h"
#include "adc.h"
#include "mq.h"
#include <string.h>

#define ESP8266_ONENET_INFO  "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"

/* Upload cadence: UPLOAD_TICKS * 25ms = 5000ms */
#define UPLOAD_TICKS  200

/*
 * Display mode — comment out to show temperature / humidity / light (default).
 * Define to show gas sensor readings: MQ2 / MQ3 / MQ4 / MQ7.
 */
#define DISPLAY_MODE_GAS

/* Sensor readings — updated every upload cycle */
u8  temp      = 0;   /* temperature, degrees C          */
u8  humi      = 0;   /* humidity, %                     */
int light_pct = 0;   /* light intensity, 0-99           */
int mq2_vol   = 0;   /* MQ2 smoke percentage, 0-100     */

/* Alarm thresholds — can be updated from cloud */
u8  temp_adj = 37;   /* temperature alarm threshold (C) */
u8  humi_adj = 90;   /* humidity alarm threshold (%)    */
int smog_th  = 50;   /* MQ2 smoke alarm threshold (%)   */
int light_th = 30;   /* light level below which LED turns on (%) */

/*
 * Actuator control mode flags
 *   1 = auto (threshold-driven)
 *   3 = cloud override (manual, preserved until next cloud command)
 */
u8 fan_mode = 1;   /* fan control mode    */
u8 led_mode = 1;   /* LED grow-light mode */

static void Hardware_Init(void);
static void Display_Init(void);
static _Bool Refresh_Sensors(void);
static void Update_Actuators(void);
static void Refresh_Display(void);

int main(void)
{
	unsigned short timeCount = 0;
	unsigned char *dataPtr   = NULL;

	Hardware_Init();
	ESP8266_Init();

	/* Connect to WiFi AP — max 20 retries, then hardware reset */
	OLED_Clear();
	OLED_ShowString(0, 0, "Connect WiFi... ", 16);
	{
		unsigned char retry = 0;
		while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
		{
			if(++retry > 20)
			{
				UsartPrintf(USART_DEBUG, "ERR: WiFi connect failed, resetting\r\n");
				NVIC_SystemReset();
			}
			DelayXms(500);
		}
	}
	OLED_ShowString(0, 3, "WiFi OK         ", 16);
	DelayXms(500);

	/* Connect to OneNET — max 10 retries, then hardware reset */
	OLED_Clear();
	OLED_ShowString(0, 0, "Device login ...", 16);
	{
		unsigned char retry = 0;
		while(OneNet_DevLink())
		{
			if(++retry > 10)
			{
				UsartPrintf(USART_DEBUG, "ERR: OneNet_DevLink failed, resetting\r\n");
				NVIC_SystemReset();
			}
			ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT");
			DelayXms(500);
		}
	}

	OneNET_Subscribe();
	timeCount = UPLOAD_TICKS - 1;   /* trigger first upload on the very first tick */
	Display_Init();

	/* Main loop — tick period 25 ms */
	while(1)
	{
		if(++timeCount >= UPLOAD_TICKS)
		{
			timeCount = 0;
			if(Refresh_Sensors())
				OneNet_SendData();   /* skip upload on DHT11 failure */
			Refresh_Display();       /* always refresh; light/MQ2 still valid */
			ESP8266_Clear();
		}

		dataPtr = ESP8266_GetIPD(0);
		if(dataPtr != NULL)
			OneNet_RevPro(dataPtr);

		Update_Actuators();

		DelayMs(25);
	}
}

/* Read all sensors into global variables.
 * Returns 1 on success, 0 if DHT11 read failed (stale values preserved). */
static _Bool Refresh_Sensors(void)
{
	uint16_t raw;
	int pct;
	_Bool ok = 1;

	if(DHT11_Read_Data(&temp, &humi) != 0)
	{
		UsartPrintf(USART_DEBUG, "WARN: DHT11 read failed, using stale values\r\n");
		ok = 0;
	}

	/* Light: ADC 0-4095 inverted to 0-99 (99 = bright) */
	raw = ADC_ConvertedValue[ADC_IDX_LIGHT];
	pct = 99 - (int)(raw / 41);
	light_pct = (pct < 0) ? 0 : (pct > 99) ? 99 : pct;

	mq2_vol = (int)Mq2_GetPercentage();

	UsartPrintf(USART_DEBUG, "T:%d H:%d Light:%d MQ2:%d%s\r\n",
	            temp, humi, light_pct, mq2_vol, ok ? "" : " [DHT11 err]");

	return ok;
}

/* Apply threshold logic; cloud overrides (mode == 3) are preserved */
static void Update_Actuators(void)
{
	/* Buzzer: environmental alarm (temperature, humidity, or smoke) */
	if((temp >= temp_adj) || (humi >= humi_adj) || (mq2_vol >= smog_th))
		Beep_Set(BEEP_ON);
	else
		Beep_Set(BEEP_OFF);

	/* Fan: temperature or humidity too high */
	if(fan_mode != 3)
	{
		if((temp >= temp_adj) || (humi >= humi_adj))
			Fan_Set(FAN_ON);
		else
			Fan_Set(FAN_OFF);
	}

	/* LED grow-light: insufficient illumination */
	if(led_mode != 3)
	{
		if(light_pct <= light_th)
			Led_Set(LED_ON);
		else
			Led_Set(LED_OFF);
	}
}

static void Refresh_Display(void)
{
	char buf[6];

#ifdef DISPLAY_MODE_GAS
	snprintf(buf, sizeof(buf), "%3d%%", Mq2_GetPercentage());
	OLED_ShowString(40, 0, buf, 16);

	snprintf(buf, sizeof(buf), "%3d%%", Mq3_GetPercentage());
	OLED_ShowString(40, 2, buf, 16);

	snprintf(buf, sizeof(buf), "%3d%%", Mq4_GetPercentage());
	OLED_ShowString(40, 4, buf, 16);

	snprintf(buf, sizeof(buf), "%3d%%", Mq7_GetPercentage());
	OLED_ShowString(40, 6, buf, 16);
#else
	snprintf(buf, sizeof(buf), "%2d", temp);
	OLED_ShowString(54, 0, buf, 16);

	snprintf(buf, sizeof(buf), "%2d", humi);
	OLED_ShowString(54, 3, buf, 16);

	snprintf(buf, sizeof(buf), "%2d", light_pct);
	OLED_ShowString(54, 6, buf, 16);
#endif
}

static void Hardware_Init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	Delay_Init();
	Usart1_Init(115200);
	Usart2_Init(115200);
	Beep_Init();
	ADCx_Init();
	OLED_Init();
	Fan_Init();

	UsartPrintf(USART_DEBUG, "Hardware init OK\r\n");
	OLED_Clear();
	OLED_ShowString(0, 0, "Hardware init OK", 16);
	DelayMs(1000);
}

static void Display_Init(void)
{
	OLED_Clear();

#ifdef DISPLAY_MODE_GAS
	/* Gas sensor mode: 4 rows, ASCII only, 16px font
	 * Layout per row: "MQx:" at col 0, value at col 40, "%" at col 56 */
	OLED_ShowString(0, 0, "MQ2:", 16);
	OLED_ShowString(0, 2, "MQ3:", 16);
	OLED_ShowString(0, 4, "MQ4:", 16);
	OLED_ShowString(0, 6, "MQ7:", 16);
#else
	/* Default mode: temperature / humidity / light */
	OLED_ShowCHinese(0,  0, 1);    /* temp char 1 */
	OLED_ShowCHinese(18, 0, 2);    /* temp char 2 */
	OLED_ShowCHinese(36, 0, 0);    /* colon       */
	OLED_ShowString( 70, 0, "C",   16);

	OLED_ShowCHinese(0,  3, 4);    /* humi char 1 */
	OLED_ShowCHinese(18, 3, 5);    /* humi char 2 */
	OLED_ShowCHinese(36, 3, 0);    /* colon       */
	OLED_ShowString( 70, 3, "%",   16);

	OLED_ShowCHinese(0,  6, 12);   /* light char 1 */
	OLED_ShowCHinese(18, 6, 13);   /* light char 2 */
	OLED_ShowCHinese(36, 6, 0);    /* colon        */
	OLED_ShowString( 70, 6, "Lux", 16);
#endif
}
