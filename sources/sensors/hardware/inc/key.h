#ifndef __KEY_H
#define __KEY_H

#include "sys.h"

/* Key driver — button on PA1, currently unused */
#define KEY1  PAin(1)

void KEY_Init(void);
u8   KEY_Scan(u8 mode);

#endif
