#include "fan.h"

uint8_t Fan_Status = FAN_OFF;

/* Fan_Init — PA8 push-pull output, initially off */
void Fan_Init(void)
{
	GPIO_InitTypeDef gpio;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin   = GPIO_Pin_8;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	Fan_Set(FAN_OFF);
}

/* Fan_Set — FAN_ON drives PA8 high, FAN_OFF drives low */
void Fan_Set(uint8_t status)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_8, status == FAN_ON ? Bit_SET : Bit_RESET);
	Fan_Status = status;
}
