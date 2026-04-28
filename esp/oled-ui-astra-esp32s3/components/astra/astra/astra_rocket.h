//
// Created by Fir on 2024/3/7 007.
// astra UI 入口（ESP32-S3 port）
//

#ifndef ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_
#define ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---- C ----*/

void astraCoreInit(void);
void astraCoreStart(void);
void astraCoreDestroy(void);

/*---- C ----*/

#ifdef __cplusplus
}

/*---- Cpp ----*/
#include "ui/launcher.h"
// Note: concrete HAL header (hal_esp32s3.h) is included only in astra_rocket.cpp

/*---- Cpp ----*/

#endif
#endif //ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_
