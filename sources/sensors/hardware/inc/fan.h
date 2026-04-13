#ifndef _FAN_H_
#define _FAN_H_

#include "stm32f10x.h"

/* FAN_ON: PA8 high; FAN_OFF: PA8 low */
#define FAN_ON   1
#define FAN_OFF  0

extern uint8_t Fan_Status;

void Fan_Init(void);
void Fan_Set(uint8_t status);

#endif
