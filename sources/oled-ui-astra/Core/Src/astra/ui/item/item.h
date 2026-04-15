/**
 * @file   item.h
 * @brief  Base Item class and Animation utility shared by all UI elements.
 * @author Fir
 * @date   2024-04-14
 */
#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_ITEM_ITEM_H_
#define ASTRA_CORE_SRC_ASTRA_UI_ITEM_ITEM_H_

#include <cmath>
#include "../../../hal/hal.h"
#include "../../../astra/config/config.h"

namespace astra {

/** @brief Base class for all UI items; holds cached system and UI config. */
class Item {
protected:
  sys::config systemConfig; ///< Cached system-level configuration.
  config astraConfig;       ///< Cached Astra UI configuration.

  /**
   * @brief  Refresh local config copies from the HAL and UI singletons.
   */
  inline void updateConfig() {
    this->systemConfig = HAL::getSystemConfig();
    this->astraConfig = getUIConfig();
  }
};

/** @brief Static helpers for screen transitions, motion interpolation, and canvas post-processing. */
class Animation {
public:
  /**
   * @brief  Play the entry transition animation (currently a no-op stub).
   */
  static void entry();

  /**
   * @brief  Play the exit fade-out transition animation.
   * @note   Not yet fully implemented; incremental pixel masking in progress.
   * @note   Step 4 applies a no-op bitwise OR; the checkerboard pattern is partially implemented.
   */
  static void exit();

  /**
   * @brief  Apply a checkerboard-pattern blur to the current canvas buffer.
   */
  static void blur();

  /**
   * @brief  Step a floating-point position toward its target using exponential decay.
   * @param  _pos     Pointer to the current position value to update.
   * @param  _posTrg  Target position to move toward.
   * @param  _speed   Animation speed in range 0–99; higher is faster.
   * @warning         Passing _speed = 100 causes division by zero.
   */
  static void move(float *_pos, float _posTrg, float _speed);
};

inline void Animation::entry() { }

// todo: exit animation not fully implemented yet.
inline void Animation::exit() {
  static unsigned char fadeFlag = 1;
  static unsigned char bufferLen = 8 * HAL::getBufferTileHeight() * HAL::getBufferTileWidth();
  auto *bufferPointer = (unsigned char *) HAL::getCanvasBuffer();

  HAL::delay(getUIConfig().fadeAnimationSpeed);

  if (getUIConfig().lightMode)
    switch (fadeFlag) {
      case 1:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] & 0xAA;
        break;
      case 2:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] & 0x00;
        break;
      case 3:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] & 0x55;
        break;
      case 4:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] & 0x00;
        break;
      default:
        // Animation finished; reset flag to allow a fresh run.
        fadeFlag = 0;
        break;
    }
  else
    switch (fadeFlag) {
      case 1:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] | 0xAA;
        break;
      case 2:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] | 0x00;
        break;
      case 3:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] | 0x55;
        break;
      case 4:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] | 0x00;
        break;
      default:
        fadeFlag = 0;
        break;
    }
  fadeFlag++;
}

inline void Animation::blur() {
  static unsigned char bufferLen = 8 * HAL::getBufferTileHeight() * HAL::getBufferTileWidth();
  static auto *bufferPointer = (unsigned char *) HAL::getCanvasBuffer();

  for (uint16_t i = 0; i < bufferLen; ++i) bufferPointer[i] = bufferPointer[i] & (i % 2 == 0 ? 0x55 : 0xAA);
}

inline void Animation::move(float *_pos, float _posTrg, float _speed) {
  if (*_pos != _posTrg) {
    if (std::fabs(*_pos - _posTrg) <= 1.0f) *_pos = _posTrg;
    else *_pos += (_posTrg - *_pos) / ((100 - _speed) / 1.0f);
  }
}
}

#endif //ASTRA_CORE_SRC_ASTRA_UI_ITEM_ITEM_H_
