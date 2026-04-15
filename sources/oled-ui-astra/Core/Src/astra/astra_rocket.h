/**
 * @file   astra_rocket.h
 * @brief  C entry points that bootstrap the Astra UI on STM32 from C code.
 * @author Fir
 * @date   2024-03-07
 */

#ifndef ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_
#define ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

// C interface

/**
 * @brief  Initialise the Astra UI subsystem (HAL inject, menu tree construction).
 */
void astraCoreInit(void);

/**
 * @brief  Enter the Astra UI main loop (blocking; does not return under normal operation).
 */
void astraCoreStart(void);

/**
 * @brief  Tear down the Astra UI subsystem and free resources.
 */
void astraCoreDestroy(void);

#ifdef __cplusplus
}

// C++ interface
#include "../astra/ui/launcher.h"
#include "../hal/hal_dreamCore/hal_dreamCore.h"

#endif
#endif //ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_
