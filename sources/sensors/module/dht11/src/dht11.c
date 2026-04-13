/** @file dht11.c
 * @brief DHT11 temperature/humidity sensor driver (STM32F103, PA0)
 */

#include "dht11.h"
#include "delay.h"

#define DT GPIO_Pin_0

/* Reset DHT11 */
void DHT11_Rst(void)
{
	DHT11_IO_OUT(); 	/* SET OUTPUT */
	DHT11_DQ_OUT(0);	/* pull DQ low */
	DelayXms(20);   	/* hold low at least 18ms */
	DHT11_DQ_OUT(1);	/* DQ=1 */
	DelayUs(30);    	/* host pulls high 20~40us */
}

/* Wait for DHT11 response.
 * Returns 1 if DHT11 not detected, 0 if present. */
uint8_t DHT11_Check(void)
{
	uint8_t retry = 0;
	DHT11_IO_IN(); /* SET INPUT */
	while (DHT11_DQ_IN && retry < 100) /* DHT11 pulls low 40~80us */
	{
		retry++;
		DelayUs(1);
	};
	if (retry >= 100) return 1;
	else retry = 0;
	while (!DHT11_DQ_IN && retry < 100) /* DHT11 pulls high again 40~80us */
	{
		retry++;
		DelayUs(1);
	};
	if (retry >= 100) return 1;
	return 0;
}

/* Read one bit from DHT11.
 * Returns 1 or 0. */
uint8_t DHT11_Read_Bit(void)
{
	uint8_t retry = 0;
	while (DHT11_DQ_IN && retry < 100) /* wait for low */
	{
		retry++;
		DelayUs(1);
	}
	retry = 0;
	while (!DHT11_DQ_IN && retry < 100) /* wait for high */
	{
		retry++;
		DelayUs(1);
	}
	DelayUs(40); /* wait 40us */
	if (DHT11_DQ_IN) return 1;
	else return 0;
}

/* Read one byte from DHT11. */
uint8_t DHT11_Read_Byte(void)
{
	uint8_t i, dat;
	dat = 0;
	for (i = 0; i < 8; i++)
	{
		dat <<= 1;
		dat |= DHT11_Read_Bit();
	}
	return dat;
}

/* Read one sample from DHT11.
 * temp: temperature (0~50 C), humi: humidity (20~90 %).
 * Returns 0 on success, 1 if sensor not responding, 2 on checksum error. */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
	uint8_t buf[5];
	uint8_t i;
	DHT11_Rst();
	if (DHT11_Check() == 0)
	{
		for (i = 0; i < 5; i++) /* read 40 bits */
		{
			buf[i] = DHT11_Read_Byte();
		}
		if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
		{
			*humi = buf[0];
			*temp = buf[2];
		}
		else return 2;   /* checksum mismatch — data not written */
	}
	else return 1;       /* sensor not responding */
	return 0;
}

/* Initialize DHT11 GPIO (PA0) and verify sensor presence.
 * Returns 1 if not found, 0 if present. */
uint8_t DHT11_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin   = DT;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, DT);

	DHT11_Rst();
	return DHT11_Check();
}
