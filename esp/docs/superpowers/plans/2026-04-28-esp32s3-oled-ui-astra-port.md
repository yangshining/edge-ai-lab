# oled-ui-astra ESP32-S3 Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the `oled-ui-astra` animated UI framework from STM32F103 to ESP32-S3 with ST7789 TFT 240×320, using u8g2 as the graphics backend (zero changes to the UI framework itself).

**Architecture:** `HAL_ESP32S3` replaces `HALDreamCore` as the concrete HAL implementation. u8g2 renders into a 1 KB 1bpp framebuffer with a no-op SPI callback; `canvasUpdate()` converts it to RGB565 and pushes it to ST7789 via `esp_lcd_panel_draw_bitmap()`. A single FreeRTOS task runs the `Launcher::update()` loop. `astra_rocket.cpp` is not compiled; its role is replaced by `esp32_rocket.cpp`.

**Tech Stack:** ESP-IDF ≥ 5.1, ESP32-S3, C++20, u8g2 (bundled), esp_lcd (ST7789), LEDC (backlight + buzzer), FreeRTOS

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `esp/CMakeLists.txt` | Create | ESP-IDF project root |
| `esp/sdkconfig.defaults` | Create | ESP32-S3 defaults (PSRAM, flash) |
| `esp/partitions.csv` | Create | Default 4 MB flash partition table |
| `esp/main/CMakeLists.txt` | Create | Component definition, SRCS, include paths |
| `esp/main/idf_component.yml` | Create | IDF version constraint (no LVGL) |
| `esp/main/Kconfig.projbuild` | Create | GPIO pin menuconfig options |
| `esp/main/driver/lcd_st7789.h` | Create | ST7789 init API (C, with extern "C" guard) |
| `esp/main/driver/lcd_st7789.c` | Create | SPI bus init, ST7789 panel handle, backlight LEDC |
| `esp/main/driver/key_gpio.h` | Create | Button debounce API (C, with extern "C" guard) |
| `esp/main/driver/key_gpio.c` | Create | GPIO init, time-based debounce state machine |
| `esp/main/hal/hal_esp32s3.h` | Create | `HAL_ESP32S3` class declaration |
| `esp/main/hal/hal_esp32s3.cpp` | Create | All 36 `HAL` virtual method overrides |
| `esp/main/esp32_rocket.cpp` | Create | HAL injection, menu tree construction, main loop |
| `esp/main/main.cpp` | Create | `app_main()`: FreeRTOS task creation |

**Not modified:** `sources/oled-ui-astra/Core/Src/astra/` (entire subtree), `sources/oled-ui-astra/Core/Src/hal/hal.h`

---

## Task 1: ESP-IDF Project Scaffold

**Files:**
- Create: `esp/CMakeLists.txt`
- Create: `esp/sdkconfig.defaults`
- Create: `esp/partitions.csv`
- Create: `esp/main/CMakeLists.txt`
- Create: `esp/main/idf_component.yml`

- [ ] **Step 1: Create project root CMakeLists.txt**

```cmake
# esp/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(oled-ui-astra-esp32s3)
```

- [ ] **Step 2: Create sdkconfig.defaults**

```ini
# esp/sdkconfig.defaults
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_FREERTOS_HZ=1000
```

Note: Do NOT set `CONFIG_IDF_TARGET` here. Run `idf.py set-target esp32s3` instead.

- [ ] **Step 3: Create partitions.csv**

```csv
# esp/partitions.csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x300000,
```

- [ ] **Step 4: Create main/idf_component.yml**

```yaml
## esp/main/idf_component.yml
dependencies:
  idf: ">=5.1.0"
```

- [ ] **Step 5: Create main/CMakeLists.txt (skeleton — SRCS filled in later tasks)**

```cmake
# esp/main/CMakeLists.txt
idf_component_register(
    SRCS
        "main.cpp"
        "esp32_rocket.cpp"
        "hal/hal_esp32s3.cpp"
        "driver/lcd_st7789.c"
        "driver/key_gpio.c"
        "astra/ui/launcher.cpp"
        "astra/ui/item/item.cpp"
        "astra/ui/item/menu/menu.cpp"
        "astra/ui/item/selector/selector.cpp"
        "astra/ui/item/camera/camera.cpp"
        "astra/ui/item/widget/widget.cpp"
        "astra/ui/item/plugin/plugin.cpp"
        "astra/app/astra_app.cpp"
        "astra/astra_logo.cpp"
    INCLUDE_DIRS
        "."
        "../../sources/oled-ui-astra/Core/Src"
    REQUIRES
        esp_lcd
        driver
        ledc
        freertos
        esp_timer
)
```

- [ ] **Step 6: Create Kconfig.projbuild**

```kconfig
# esp/main/Kconfig.projbuild
menu "oled-ui-astra ESP32-S3 Config"

    config LCD_GPIO_SCK
        int "LCD SPI SCK GPIO"
        default 18

    config LCD_GPIO_MOSI
        int "LCD SPI MOSI GPIO"
        default 17

    config LCD_GPIO_CS
        int "LCD SPI CS GPIO"
        default 4

    config LCD_GPIO_DC
        int "LCD Data/Command GPIO"
        default 5

    config LCD_GPIO_RST
        int "LCD Reset GPIO (avoid GPIO3 on DevKitC-1)"
        default 48

    config LCD_GPIO_BL
        int "LCD Backlight PWM GPIO"
        default 2

    config KEY0_GPIO
        int "KEY_0 (Confirm) GPIO — active low, internal pull-up"
        default 6

    config KEY1_GPIO
        int "KEY_1 (Back) GPIO — active low, internal pull-up"
        default 7

endmenu
```

- [ ] **Step 7: Create astra/ symlink or copy**

The `astra/` subdirectory must resolve to `sources/oled-ui-astra/Core/Src/astra/` relative to the project root. Use a symlink on Linux/macOS or a directory junction on Windows:

```bash
# from esp/main/
# Windows (run as administrator or in Developer Mode):
cmd /c mklink /J astra ..\..\sources\oled-ui-astra\Core\Src\astra
# Linux/macOS:
# ln -s ../../sources/oled-ui-astra/Core/Src/astra astra
```

Alternatively, copy the directory. The `astra_rocket.cpp` and `astra_rocket.h` files inside the copy must NOT be listed in `CMakeLists.txt SRCS` (they are excluded by not listing them — ESP-IDF does not auto-glob).

- [ ] **Step 8: Verify IDF target is set**

```bash
cd esp
idf.py set-target esp32s3
```

Expected: `sdkconfig` file created/updated with `CONFIG_IDF_TARGET="esp32s3"`.

- [ ] **Step 9: Commit scaffold**

```bash
git add esp/CMakeLists.txt esp/sdkconfig.defaults esp/partitions.csv
git add esp/main/CMakeLists.txt esp/main/idf_component.yml esp/main/Kconfig.projbuild
git commit -m "[esp] scaffold: ESP-IDF project structure for ESP32-S3 port"
```

---

## Task 2: LCD Driver (ST7789 + Backlight)

**Files:**
- Create: `esp/main/driver/lcd_st7789.h`
- Create: `esp/main/driver/lcd_st7789.c`

This driver initializes the SPI2 bus, creates an ST7789 panel handle, and provides the panel handle to callers. Backlight is controlled via LEDC channel 0.

- [ ] **Step 1: Create lcd_st7789.h**

```c
// esp/main/driver/lcd_st7789.h
#pragma once
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise SPI2 bus, ST7789 panel, and LEDC backlight.
 * Must be called before any draw operations.
 * Returns the panel handle (caller must not free it).
 */
esp_lcd_panel_handle_t lcd_st7789_init(void);

/** Set backlight brightness: 0 (off) to 255 (full). */
void lcd_st7789_set_backlight(uint8_t brightness);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Create lcd_st7789.c**

```c
// esp/main/driver/lcd_st7789.c
#include "lcd_st7789.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#define LCD_H_RES       240
#define LCD_V_RES       320
#define LCD_SPI_CLOCK   40000000   // 40 MHz

#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY  5000

static const char *TAG = "lcd_st7789";

esp_lcd_panel_handle_t lcd_st7789_init(void) {
    // LEDC backlight
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num   = CONFIG_LCD_GPIO_BL,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));

    // SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = CONFIG_LCD_GPIO_MOSI,
        .miso_io_num     = -1,
        .sclk_io_num     = CONFIG_LCD_GPIO_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // Panel IO
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = CONFIG_LCD_GPIO_DC,
        .cs_gpio_num       = CONFIG_LCD_GPIO_CS,
        .pclk_hz           = LCD_SPI_CLOCK,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io_handle));

    // ST7789 panel
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = CONFIG_LCD_GPIO_RST,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    ESP_LOGI(TAG, "ST7789 initialized (%dx%d)", LCD_H_RES, LCD_V_RES);
    return panel;
}

void lcd_st7789_set_backlight(uint8_t brightness) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}
```

- [ ] **Step 3: Verify it compiles (build will fail on missing symbols — that's OK at this stage)**

```bash
cd esp
idf.py build 2>&1 | head -50
```

Expected: Errors about missing `main.cpp`, `hal_esp32s3.cpp`, etc. — NOT errors in `lcd_st7789.c` itself. If you see errors in `lcd_st7789.c`, fix them before proceeding.

- [ ] **Step 4: Commit**

```bash
git add esp/main/driver/lcd_st7789.h esp/main/driver/lcd_st7789.c
git commit -m "[esp] add ST7789 LCD driver with LEDC backlight"
```

---

## Task 3: GPIO Key Driver

**Files:**
- Create: `esp/main/driver/key_gpio.h`
- Create: `esp/main/driver/key_gpio.c`

Two buttons, active low with internal pull-up. Time-based debounce using `esp_timer_get_time()`. Populates the per-key state so `HAL_ESP32S3::_keyScan()` can call `key_gpio_scan()` and write `HAL::key[]`.

- [ ] **Step 1: Create key_gpio.h**

```c
// esp/main/driver/key_gpio.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_NUM 2

typedef enum {
    KEY_ACTION_NONE  = 0,
    KEY_ACTION_CLICK = 1,
    KEY_ACTION_LONG  = 2,
} key_action_t;

/**
 * Initialise GPIO inputs for both keys.
 * Must be called once before key_gpio_scan().
 */
void key_gpio_init(void);

/**
 * Update debounce state for all keys.
 * Call every frame from HAL_ESP32S3::_keyScan().
 * Results are read via key_gpio_get_action().
 */
void key_gpio_scan(void);

/**
 * Return the current action for key index (0 or 1).
 * Action is consumed (reset to NONE) after reading.
 */
key_action_t key_gpio_get_action(uint8_t index);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Create key_gpio.c**

```c
// esp/main/driver/key_gpio.c
#include "key_gpio.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DEBOUNCE_MS     20
#define LONG_PRESS_MS   600

static const int KEY_GPIOS[KEY_NUM] = {
    CONFIG_KEY0_GPIO,
    CONFIG_KEY1_GPIO,
};

typedef struct {
    bool         pressed;
    int64_t      press_time_us;
    key_action_t action;
} key_state_t;

static key_state_t s_keys[KEY_NUM];

void key_gpio_init(void) {
    for (int i = 0; i < KEY_NUM; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << KEY_GPIOS[i],
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        s_keys[i] = (key_state_t){false, 0, KEY_ACTION_NONE};
    }
}

void key_gpio_scan(void) {
    int64_t now_us = esp_timer_get_time();
    for (int i = 0; i < KEY_NUM; i++) {
        bool raw_pressed = (gpio_get_level(KEY_GPIOS[i]) == 0);  // active low
        if (raw_pressed && !s_keys[i].pressed) {
            // Debounce: wait DEBOUNCE_MS before registering press
            if (s_keys[i].press_time_us == 0) {
                s_keys[i].press_time_us = now_us;
            } else if ((now_us - s_keys[i].press_time_us) >= DEBOUNCE_MS * 1000) {
                s_keys[i].pressed = true;
            }
        } else if (!raw_pressed && s_keys[i].pressed) {
            // Key released
            int64_t held_ms = (now_us - s_keys[i].press_time_us) / 1000;
            s_keys[i].action   = (held_ms >= LONG_PRESS_MS) ? KEY_ACTION_LONG : KEY_ACTION_CLICK;
            s_keys[i].pressed  = false;
            s_keys[i].press_time_us = 0;
        } else if (!raw_pressed) {
            s_keys[i].press_time_us = 0;
        }
    }
}

key_action_t key_gpio_get_action(uint8_t index) {
    key_action_t a = s_keys[index].action;
    s_keys[index].action = KEY_ACTION_NONE;
    return a;
}
```

- [ ] **Step 3: Commit**

```bash
git add esp/main/driver/key_gpio.h esp/main/driver/key_gpio.c
git commit -m "[esp] add GPIO button driver with debounce"
```

---

## Task 4: HAL_ESP32S3 — Header

**Files:**
- Create: `esp/main/hal/hal_esp32s3.h`

This class inherits from `HAL` (defined in `sources/oled-ui-astra/Core/Src/hal/hal.h`) and overrides all 33 pure-virtual methods.

- [ ] **Step 1: Create hal_esp32s3.h**

```cpp
// esp/main/hal/hal_esp32s3.h
#pragma once
#include "hal/hal.h"
#include "esp_lcd_panel_ops.h"

// Forward declaration — u8g2.h included only in .cpp
struct u8g2_struct;
typedef struct u8g2_struct u8g2_t;

class HAL_ESP32S3 : public HAL {
public:
    HAL_ESP32S3() = default;
    ~HAL_ESP32S3() override = default;

    void init() override;

protected:
    // --- Canvas ---
    unsigned char *_getCanvasBuffer() override;
    unsigned char  _getBufferTileHeight() override;
    unsigned char  _getBufferTileWidth() override;
    void           _canvasUpdate() override;
    void           _canvasClear() override;

    // --- Font & Drawing ---
    void _setFont(const unsigned char *_font) override;
    unsigned int  _getFontWidth(std::string &_text) override;
    unsigned char _getFontHeight() override;
    void _setDrawType(unsigned char _type) override;
    void _drawPixel(float _x, float _y) override;
    void _drawEnglish(float _x, float _y, const std::string &_text) override;
    void _drawChinese(float _x, float _y, const std::string &_text) override;
    void _drawVDottedLine(float _x, float _y, float _h) override;
    void _drawHDottedLine(float _x, float _y, float _l) override;
    void _drawVLine(float _x, float _y, float _h) override;
    void _drawHLine(float _x, float _y, float _l) override;
    void _drawBMP(float _x, float _y, float _w, float _h, const unsigned char *_bitMap) override;
    void _drawBox(float _x, float _y, float _w, float _h) override;
    void _drawRBox(float _x, float _y, float _w, float _h, float _r) override;
    void _drawFrame(float _x, float _y, float _w, float _h) override;
    void _drawRFrame(float _x, float _y, float _w, float _h, float _r) override;

    // --- Timing ---
    void          _delay(unsigned long _mill) override;
    unsigned long _millis() override;
    unsigned long _getTick() override;
    unsigned long _getRandomSeed() override;

    // --- Audio ---
    void _beep(float _freq) override;
    void _beepStop() override;
    void _setBeepVol(unsigned char _vol) override;

    // --- Screen power ---
    void _screenOn() override;
    void _screenOff() override;

    // --- Input ---
    key::KEY_ACTION _getKey(key::KEY_INDEX _keyIndex) override;
    void _keyScan() override;
    void _keyTest() override;

    // --- Config ---
    void _updateConfig() override;

private:
    u8g2_t              *_u8g2 = nullptr;
    esp_lcd_panel_handle_t _panel = nullptr;

    // 128x64 pixels, 2 bytes per pixel RGB565 = 16384 bytes
    // Static to avoid stack overflow in astra_task (16 KB stack guard)
    static uint16_t _rgb565_buf[128 * 64];
};
```

- [ ] **Step 2: Commit**

```bash
git add esp/main/hal/hal_esp32s3.h
git commit -m "[esp] add HAL_ESP32S3 class declaration"
```

---

## Task 5: HAL_ESP32S3 — Implementation

**Files:**
- Create: `esp/main/hal/hal_esp32s3.cpp`

The most substantial file. Implements all virtual methods. u8g2 uses a no-op SPI callback; the RGB565 static buffer is written in `_canvasUpdate()`.

- [ ] **Step 1: Create hal_esp32s3.cpp**

```cpp
// esp/main/hal/hal_esp32s3.cpp
#include "hal_esp32s3.h"
#include "driver/lcd_st7789.h"
#include "driver/key_gpio.h"
#include "hal/hal_dreamCore/components/oled/graph_lib/u8g2/u8g2.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "esp_random.h"

// ── Statics ──────────────────────────────────────────────────────────────────

uint16_t HAL_ESP32S3::_rgb565_buf[128 * 64];

// ── u8g2 no-op callbacks ──────────────────────────────────────────────────────
// u8g2's SPI transport is unused. All draw calls write to the internal RAM
// framebuffer. _canvasUpdate() reads the buffer directly via u8g2_GetBufferPtr().

static uint8_t u8g2_noop_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    return 1;
}

static uint8_t u8g2_noop_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    if (msg == U8X8_MSG_DELAY_MILLI) {
        vTaskDelay(pdMS_TO_TICKS(arg_int));
    }
    return 1;
}

// ── HAL_ESP32S3::init() ───────────────────────────────────────────────────────

void HAL_ESP32S3::init() {
    // LCD + backlight
    _panel = lcd_st7789_init();
    lcd_st7789_set_backlight(255);

    // GPIO keys
    key_gpio_init();

    // u8g2: full-buffer SSD1306 128x64 mode with no-op SPI callbacks
    _u8g2 = new u8g2_t;
    u8g2_Setup_ssd1306_128x64_noname_f(
        _u8g2,
        U8G2_R0,
        u8g2_noop_byte_cb,
        u8g2_noop_gpio_delay_cb
    );
    u8g2_InitDisplay(_u8g2);
    u8g2_SetPowerSave(_u8g2, 0);
    u8g2_ClearBuffer(_u8g2);

    // Populate system config (logical canvas dimensions, not physical TFT)
    // NOTE: field is "screenWeight" (typo in original codebase) — do NOT rename
    config.screenWeight = 128;
    config.screenHeight = 64;
    config.screenBright = 255;
}

// ── Canvas ────────────────────────────────────────────────────────────────────

unsigned char *HAL_ESP32S3::_getCanvasBuffer() {
    return u8g2_GetBufferPtr(_u8g2);
}

unsigned char HAL_ESP32S3::_getBufferTileHeight() {
    return u8g2_GetBufferTileHeight(_u8g2);
}

unsigned char HAL_ESP32S3::_getBufferTileWidth() {
    return u8g2_GetBufferTileWidth(_u8g2);
}

void HAL_ESP32S3::_canvasUpdate() {
    // Step 1: flush u8g2 internal state (no-op SPI callback — no actual SPI traffic)
    u8g2_SendBuffer(_u8g2);

    // Step 2: read 1bpp framebuffer
    const uint8_t *src = u8g2_GetBufferPtr(_u8g2);

    // Step 3: convert 1bpp → RGB565
    // u8g2 packs 8 pixels per byte, MSB = leftmost pixel
    // 0xFFFF = white, 0x0000 = black (byte-order-neutral for B&W)
    uint16_t *dst = _rgb565_buf;
    for (int page = 0; page < 8; page++) {          // 8 pages × 8 rows = 64 rows
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 16; col++) {    // 16 bytes × 8 bits = 128 cols
                uint8_t byte = src[page * 128 + col];
                for (int bit = 7; bit >= 0; bit--) {
                    *dst++ = (byte & (1 << bit)) ? 0xFFFFu : 0x0000u;
                }
            }
        }
    }

    // Step 4: push to ST7789 top-left corner
    esp_lcd_panel_draw_bitmap(_panel, 0, 0, 128, 64, _rgb565_buf);
}

void HAL_ESP32S3::_canvasClear() {
    u8g2_ClearBuffer(_u8g2);
}

// ── Font & Drawing ────────────────────────────────────────────────────────────

void HAL_ESP32S3::_setFont(const unsigned char *_font) {
    u8g2_SetFont(_u8g2, _font);
}

unsigned int HAL_ESP32S3::_getFontWidth(std::string &_text) {
    return u8g2_GetUTF8Width(_u8g2, _text.c_str());
}

unsigned char HAL_ESP32S3::_getFontHeight() {
    return u8g2_GetMaxCharHeight(_u8g2);
}

void HAL_ESP32S3::_setDrawType(unsigned char _type) {
    u8g2_SetDrawColor(_u8g2, _type);
}

void HAL_ESP32S3::_drawPixel(float _x, float _y) {
    u8g2_DrawPixel(_u8g2, (int)_x, (int)_y);
}

void HAL_ESP32S3::_drawEnglish(float _x, float _y, const std::string &_text) {
    u8g2_DrawUTF8(_u8g2, (int)_x, (int)_y, _text.c_str());
}

void HAL_ESP32S3::_drawChinese(float _x, float _y, const std::string &_text) {
    u8g2_DrawUTF8(_u8g2, (int)_x, (int)_y, _text.c_str());
}

void HAL_ESP32S3::_drawVDottedLine(float _x, float _y, float _h) {
    for (float i = _y; i < _y + _h; i += 2)
        u8g2_DrawPixel(_u8g2, (int)_x, (int)i);
}

void HAL_ESP32S3::_drawHDottedLine(float _x, float _y, float _l) {
    for (float i = _x; i < _x + _l; i += 2)
        u8g2_DrawPixel(_u8g2, (int)i, (int)_y);
}

void HAL_ESP32S3::_drawVLine(float _x, float _y, float _h) {
    u8g2_DrawVLine(_u8g2, (int)_x, (int)_y, (int)_h);
}

void HAL_ESP32S3::_drawHLine(float _x, float _y, float _l) {
    u8g2_DrawHLine(_u8g2, (int)_x, (int)_y, (int)_l);
}

void HAL_ESP32S3::_drawBMP(float _x, float _y, float _w, float _h, const unsigned char *_bitMap) {
    u8g2_DrawXBMP(_u8g2, (int)_x, (int)_y, (int)_w, (int)_h, _bitMap);
}

void HAL_ESP32S3::_drawBox(float _x, float _y, float _w, float _h) {
    u8g2_DrawBox(_u8g2, (int)_x, (int)_y, (int)_w, (int)_h);
}

void HAL_ESP32S3::_drawRBox(float _x, float _y, float _w, float _h, float _r) {
    u8g2_DrawRBox(_u8g2, (int)_x, (int)_y, (int)_w, (int)_h, (int)_r);
}

void HAL_ESP32S3::_drawFrame(float _x, float _y, float _w, float _h) {
    u8g2_DrawFrame(_u8g2, (int)_x, (int)_y, (int)_w, (int)_h);
}

void HAL_ESP32S3::_drawRFrame(float _x, float _y, float _w, float _h, float _r) {
    u8g2_DrawRFrame(_u8g2, (int)_x, (int)_y, (int)_w, (int)_h, (int)_r);
}

// ── Timing ────────────────────────────────────────────────────────────────────

void HAL_ESP32S3::_delay(unsigned long _mill) {
    vTaskDelay(pdMS_TO_TICKS(_mill));
}

unsigned long HAL_ESP32S3::_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

unsigned long HAL_ESP32S3::_getTick() {
    return (unsigned long)xTaskGetTickCount();
}

unsigned long HAL_ESP32S3::_getRandomSeed() {
    return (unsigned long)esp_random();
}

// ── Audio ─────────────────────────────────────────────────────────────────────
// Buzzer on LEDC channel 1. LEDC channel 0 is used by backlight (lcd_st7789.c).

#define BUZZER_LEDC_CHANNEL  LEDC_CHANNEL_1
#define BUZZER_LEDC_TIMER    LEDC_TIMER_1
#define BUZZER_GPIO          CONFIG_LCD_GPIO_BL   // replace with buzzer GPIO if wired

void HAL_ESP32S3::_beep(float _freq) {
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = BUZZER_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz         = (uint32_t)_freq,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 512); // 50% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

void HAL_ESP32S3::_beepStop() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

void HAL_ESP32S3::_setBeepVol(unsigned char _vol) {
    // Map 0-255 volume to 0-512 duty (10-bit timer)
    uint32_t duty = ((uint32_t)_vol * 512) / 255;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

// ── Screen power ──────────────────────────────────────────────────────────────

void HAL_ESP32S3::_screenOn() {
    lcd_st7789_set_backlight(255);
}

void HAL_ESP32S3::_screenOff() {
    lcd_st7789_set_backlight(0);
}

// ── Input ─────────────────────────────────────────────────────────────────────

void HAL_ESP32S3::_keyScan() {
    key_gpio_scan();

    for (int i = 0; i < KEY_NUM; i++) {
        key_action_t a = key_gpio_get_action((uint8_t)i);
        switch (a) {
            case KEY_ACTION_CLICK:
                key[i] = key::KEY_ACTION::KEY_ACTION_CLICK;
                keyFlag = static_cast<key::KEY_TYPE>(i);
                break;
            case KEY_ACTION_LONG:
                key[i] = key::KEY_ACTION::KEY_ACTION_LONG;
                keyFlag = static_cast<key::KEY_TYPE>(i);
                break;
            default:
                key[i] = static_cast<key::KEY_ACTION>(0);
                break;
        }
    }
}

key::KEY_ACTION HAL_ESP32S3::_getKey(key::KEY_INDEX _keyIndex) {
    return key[static_cast<int>(_keyIndex)];
}

void HAL_ESP32S3::_keyTest() {
    // Optional: implement key test sequence if needed
}

// ── Config ────────────────────────────────────────────────────────────────────

void HAL_ESP32S3::_updateConfig() {
    // Config is set in init() and does not change at runtime in this port.
    // Brightness changes could update backlight here if needed.
}
```

- [ ] **Step 2: Verify the 1bpp→RGB565 conversion loop logic**

The u8g2 framebuffer for `ssd1306_128x64_noname_f` is organized as 8 pages × 128 bytes. Each byte contains 8 vertically-stacked pixels for one column. The conversion loop must produce a row-major RGB565 buffer for `esp_lcd_panel_draw_bitmap()`.

Verify: for page `p`, row `r` (0–7), column byte `b` (0–15, each byte = 8 columns):
- Source byte index: `src[p * 128 + b * 8 + col_within_byte]` — but the actual layout is simpler: `src[p * 128 + col_x]` where bit `r` of that byte is the pixel at row `p*8 + r`, col `col_x`.

The correct conversion loop:

```cpp
// Correct layout: src[page * 128 + x] bit r = pixel at (x, page*8 + r)
uint16_t *dst = _rgb565_buf;
for (int y = 0; y < 64; y++) {
    int page = y / 8;
    int bit  = y % 8;
    for (int x = 0; x < 128; x++) {
        uint8_t byte = src[page * 128 + x];
        *dst++ = (byte >> bit) & 1 ? 0xFFFFu : 0x0000u;
    }
}
```

Replace the loop in `_canvasUpdate()` with the corrected version above.

- [ ] **Step 3: Commit**

```bash
git add esp/main/hal/hal_esp32s3.h esp/main/hal/hal_esp32s3.cpp
git commit -m "[esp] implement HAL_ESP32S3 with u8g2 backend and ST7789 output"
```

---

## Task 6: esp32_rocket.cpp and main.cpp

**Files:**
- Create: `esp/main/esp32_rocket.cpp`
- Create: `esp/main/main.cpp`

`esp32_rocket.cpp` replaces `astra_rocket.cpp` — injects `HAL_ESP32S3`, builds the demo menu tree, drives the main loop. `main.cpp` is the FreeRTOS entry point.

- [ ] **Step 1: Create esp32_rocket.cpp**

```cpp
// esp/main/esp32_rocket.cpp
// Replaces sources/oled-ui-astra/Core/Src/astra/astra_rocket.cpp for ESP32.
// Does NOT include astra_rocket.h or hal_dreamCore.h.
#include "astra/ui/launcher.h"
#include "astra/ui/item/menu/menu.h"
#include "astra/ui/item/widget/widget.h"
#include "astra/astra_logo.h"
#include "hal/hal_esp32s3.h"

static auto *s_launcher  = new astra::Launcher();
static auto *s_rootPage  = new astra::Tile("root");
static auto *s_secondPage = new astra::List("secondPage");

// Widget state
static bool          s_test      = false;
static unsigned char s_testIndex = 0;
static unsigned char s_testSlider = 60;

void esp32_astra_init(void) {
    HAL::inject(new HAL_ESP32S3());

    HAL::get()->init();
    HAL::delay(350);
    astra::drawLogo(1000);

    s_rootPage->addItem(new astra::List("test1"));
    s_rootPage->addItem(new astra::List("测试2"));
    s_rootPage->addItem(new astra::List("测试测试3"));
    s_rootPage->addItem(new astra::List("测试测试3"));
    s_rootPage->addItem(s_secondPage);

    s_secondPage->addItem(new astra::List());
    s_secondPage->addItem(new astra::List("-测试2"), new astra::CheckBox(s_test));
    s_secondPage->addItem(new astra::Tile("-测试测试3"),
        new astra::PopUp(1, "测试", {"测试"}, s_testIndex));
    s_secondPage->addItem(new astra::Tile("-测试测试测试4"),
        new astra::Slider("测试", 0, 100, 50, s_testSlider));
    s_secondPage->addItem(new astra::List("-测试测试测试5"));

    s_launcher->init(s_rootPage);
}

void esp32_astra_start(void) {
    for (;;) {  //NOLINT
        s_launcher->update();
    }
}
```

- [ ] **Step 2: Create main.cpp**

```cpp
// esp/main/main.cpp
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern void esp32_astra_init(void);
extern void esp32_astra_start(void);

static void astra_task(void *) {
    esp32_astra_init();
    esp32_astra_start();   // does not return
    vTaskDelete(nullptr);
}

extern "C" void app_main(void) {
    xTaskCreate(astra_task, "astra", 16384, nullptr, 3, nullptr);
}
```

- [ ] **Step 3: Commit**

```bash
git add esp/main/esp32_rocket.cpp esp/main/main.cpp
git commit -m "[esp] add esp32_rocket and main entry point"
```

---

## Task 7: First Build & Fix Compilation Errors

At this point all files exist. The first build will likely expose include-path issues, missing `extern "C"` guards, or u8g2 API differences.

- [ ] **Step 1: Attempt full build**

```bash
cd esp
idf.py build 2>&1 | tee /tmp/build_output.txt
```

- [ ] **Step 2: Fix include-path errors**

Common issues and fixes:
- `hal.h not found` → verify `INCLUDE_DIRS` in `main/CMakeLists.txt` points to `../../sources/oled-ui-astra/Core/Src`
- `u8g2.h not found` → the include path for u8g2 is `hal/hal_dreamCore/components/oled/graph_lib/u8g2/u8g2.h` — add the u8g2 directory to `INCLUDE_DIRS` or use the full relative path in the `#include`
- STM32 headers pulled in via `hal.h` → open `hal.h` and check if it conditionally includes STM32 headers; if so, add a guard macro

- [ ] **Step 3: Fix u8g2 setup function signature if needed**

The u8g2 bundled version may have a slightly different `u8g2_Setup_ssd1306_128x64_noname_f` signature. Check the bundled `u8g2.h` and adjust the call in `hal_esp32s3.cpp` if needed.

- [ ] **Step 4: Fix any C++ standard issues**

ESP-IDF defaults to C++17 for components. The astra framework requires C++20. Add to `main/CMakeLists.txt`:

```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20)
```

- [ ] **Step 5: Verify binary size**

```bash
idf.py size
```

Expected: firmware fits in the 3 MB factory partition. If not, check for debug symbols or unnecessary components.

- [ ] **Step 6: Commit fixes**

```bash
git add -p   # stage only your fixes
git commit -m "[esp] fix build errors in first compilation pass"
```

---

## Task 8: Flash & Smoke Test

- [ ] **Step 1: Flash to board**

```bash
cd esp
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with your serial port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux).

- [ ] **Step 2: Verify splash screen appears**

Expected: `astra::drawLogo()` displays the logo bitmap on ST7789 for ~1 second after boot. If the screen stays black, check:
- Backlight: `lcd_st7789_set_backlight(255)` called?
- `esp_lcd_panel_disp_on_off(panel, true)` called?
- SPI GPIO numbers match physical wiring (check Kconfig values vs. board)

- [ ] **Step 3: Verify main menu appears**

Expected: After the splash, the root Tile menu appears with "test1", "测试2", etc. items. Animated transitions should be visible.

- [ ] **Step 4: Verify button navigation**

Press KEY_0 (confirm) to enter a submenu. Press KEY_1 (back) to return. If buttons don't respond:
- Check `CONFIG_KEY0_GPIO` / `CONFIG_KEY1_GPIO` match physical wiring
- Add a `printf` in `key_gpio_scan()` temporarily to verify raw GPIO reads

- [ ] **Step 5: Check for stack overflow**

In `esp32_rocket.cpp`'s `esp32_astra_start()` loop, add a temporary check:

```cpp
UBaseType_t watermark = uxTaskGetHighWaterMark(nullptr);
printf("stack watermark: %lu bytes\n", (unsigned long)(watermark * sizeof(StackType_t)));
```

If watermark < 1024, increase `xTaskCreate` stack size by 8 KB and re-flash.

- [ ] **Step 6: Commit smoke test results / fixes**

```bash
git commit -am "[esp] smoke test pass: menu renders, buttons navigate"
```

---

## Done

At this point the port is functional: oled-ui-astra animated menus run on ESP32-S3 with ST7789 TFT, driven by u8g2 with GPIO button input.

**Next steps (out of scope for this plan):**
- Add XPT2046 touch input via `esp_lcd_touch`
- Upgrade rendering to full LVGL color pipeline
- Add NVS settings persistence
