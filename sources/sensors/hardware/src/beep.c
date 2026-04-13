#include "beep.h"

uint8_t Beep_Status = BEEP_OFF;
uint8_t Co_Status   = 0;

/*
 * Beep_Init — initialize PC13/PC14/PC15 as push-pull outputs.
 *   PC13: buzzer (active low)
 *   PC14: LED grow-light (active low)
 *   PC15: irrigation (active low, spare)
 */
void Beep_Init(void)
{
	GPIO_InitTypeDef gpio;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &gpio);

	Beep_Set(BEEP_OFF);
	Led_Set(LED_OFF);
	Irrigation_Set(0);
}

/* Buzzer: PC13, active low */
void Beep_Set(uint8_t status)
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, status == BEEP_ON ? Bit_RESET : Bit_SET);
	Beep_Status = status;
}

/* LED grow-light: PC14, active low */
void Led_Set(uint8_t status)
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_14, status ? Bit_RESET : Bit_SET);
}

/* Irrigation: PC15, active low */
void Irrigation_Set(uint8_t status)
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_15, status ? Bit_RESET : Bit_SET);
}
