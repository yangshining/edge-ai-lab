#ifndef _BEEP_H_
#define _BEEP_H_

#include "stm32f10x.h"

#define BEEP_ON   1
#define BEEP_OFF  0

#define LED_ON    1
#define LED_OFF   0

extern uint8_t Beep_Status;
extern uint8_t Co_Status;

void Beep_Init(void);
void Beep_Set(uint8_t status);

/* PC14: LED grow-light control */
void Led_Set(uint8_t status);

/* PC15: irrigation control (spare) */
void Irrigation_Set(uint8_t status);

#endif
