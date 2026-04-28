//
// HALEspCore — key input methods
//
// Both keys are active-low with internal pull-ups enabled in _gpio_init().
//

#include "hal_esp32s3.h"
#include "driver/gpio.h"

bool HALEspCore::_getKey(key::KEY_INDEX _keyIndex) {
    if (_keyIndex == key::KEY_0) return gpio_get_level(PIN_KEY0) == 0;
    if (_keyIndex == key::KEY_1) return gpio_get_level(PIN_KEY1) == 0;
    return false;
}
