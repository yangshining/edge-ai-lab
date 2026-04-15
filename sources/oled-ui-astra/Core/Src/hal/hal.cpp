/**
 * @file   hal.cpp
 * @brief  HAL singleton lifecycle and default key/display helper implementations.
 * @author Fir
 * @date   2024-02-08
 */

#include <cstring>
#include "hal.h"

HAL *HAL::hal = nullptr;

HAL *HAL::get() {
  return hal;
}

bool HAL::check() {
  return hal != nullptr;
}

bool HAL::inject(HAL *_hal) {
  if (_hal == nullptr) {
    return false;
  }

  _hal->init();
  hal = _hal;
  return true;
}

void HAL::destroy() {
  if (hal == nullptr) return;

  delete hal;
  hal = nullptr;
}

// Prints a log message to the display with auto-wrapping; do not call in a tight loop.
void HAL::_printInfo(std::string _msg) {
  static std::vector<std::string> _infoCache = {};
  static const unsigned char _max = getSystemConfig().screenHeight / getFontHeight();
  static const unsigned char _fontHeight = getFontHeight();

  if (_infoCache.size() >= _max) _infoCache.clear();
  _infoCache.push_back(_msg);

  canvasClear();
  setDrawType(2); // inverted color rendering
  for (unsigned char i = 0; i < _infoCache.size(); i++) {
    drawEnglish(0, _fontHeight + i * (1 + _fontHeight), _infoCache[i]);
  }
  canvasUpdate();
  setDrawType(1); // normal color rendering
}

bool HAL::_getAnyKey() {
  for (int i = 0; i < key::KEY_NUM; i++) {
    if (getKey(static_cast<key::KEY_INDEX>(i))) return true;
  }
  return false;
}

// Default key scanner; call every 10 ms to detect clicks and long-presses.
void HAL::_keyScan() {
  static unsigned char _timeCnt = 0;
  // _lock: true once a key has been held long enough; suppresses the release from registering as a click.
  static bool _lock = false;
  static key::KEY_FILTER _keyFilter = key::CHECKING;
  switch (_keyFilter) {
    case key::CHECKING:
      if (getAnyKey()) {
        if (getKey(key::KEY_0)) _keyFilter = key::KEY_0_CONFIRM;
        if (getKey(key::KEY_1)) _keyFilter = key::KEY_1_CONFIRM;
      }
      _timeCnt = 0;
      _lock = false;
      break;

    case key::KEY_0_CONFIRM:
    case key::KEY_1_CONFIRM:
      // hold-time accumulator: count ticks while held to distinguish click from long-press
      if (getAnyKey()) {
        if (!_lock) _lock = true;
        _timeCnt++;

        if (_timeCnt > 100) {
          keyFlag = key::KEY_PRESSED;
          // long-press threshold: >100 ticks @ 10 ms/tick = ~1 s
          if (getKey(key::KEY_0)) {
            key[key::KEY_0] = key::PRESS;
            key[key::KEY_1] = key::INVALID;
          }
          if (getKey(key::KEY_1)) {
            key[key::KEY_1] = key::PRESS;
            key[key::KEY_0] = key::INVALID;
          }
          _timeCnt = 0;
          _lock = false;
          _keyFilter = key::RELEASED;
        }
      } else {
        if (_lock) {
          if (_keyFilter == key::KEY_0_CONFIRM) {
            key[key::KEY_0] = key::CLICK;
            key[key::KEY_1] = key::INVALID;
          }
          if (_keyFilter == key::KEY_1_CONFIRM) {
            key[key::KEY_1] = key::CLICK;
            key[key::KEY_0] = key::INVALID;
          }
          keyFlag = key::KEY_PRESSED;
          _keyFilter = key::RELEASED;
        } else {
          _keyFilter = key::CHECKING;
          key[key::KEY_0] = key::INVALID;
          key[key::KEY_1] = key::INVALID;
        }
      }
      break;

    case key::RELEASED:
      if (!getAnyKey()) _keyFilter = key::CHECKING;
      break;

    default: break;
  }
}

// Default key test handler stub; unconditionally clears key state when any key is active.
void HAL::_keyTest() {
  if (getAnyKey()) {
    for (unsigned char i = 0; i < key::KEY_NUM; i++) {
      if (key[i] == key::CLICK) {
        if (i == 0) break;
        if (i == 1) break;
      } else if (key[i] == key::PRESS) {
        if (i == 0) break;
        if (i == 1) break;
      }
    }
    memset(key, key::INVALID, sizeof(key));
  }
}
