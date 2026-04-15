/**
 * @file   astra_logo.h
 * @brief  Splash-screen logo drawing function for the Astra UI.
 * @author Fir
 * @date   2024-03-21
 */

#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_ASTRA_LOGO_H_
#define ASTRA_CORE_SRC_ASTRA_ASTRA_LOGO_H_

#include "../hal/hal.h"

namespace astra {

/**
 * @brief  Draw the Astra logo splash screen and hold it for the given duration.
 * @param  _time  Display duration in milliseconds.
 */
void drawLogo(uint16_t _time);

}

#endif //ASTRA_CORE_SRC_ASTRA_ASTRA_LOGO_H_
