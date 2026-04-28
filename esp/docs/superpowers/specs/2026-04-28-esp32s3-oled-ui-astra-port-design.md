# Design Spec: Port oled-ui-astra to ESP32-S3

**Date:** 2026-04-28  
**Status:** Revised  
**Working directory:** `edge-ai-lab/esp/`

---

## 1. Goal

Port the `oled-ui-astra` C++ animated UI framework from STM32F103 (SSD1306 OLED, 128×64, bare metal) to ESP32-S3 (ST7789 TFT, 240×320, ESP-IDF / FreeRTOS), with minimal changes to the UI framework itself.

## 2. Constraints & Decisions

| Question | Decision |
|---|---|
| Target display | ST7789 TFT, 240×320, SPI (identical to `tft_touch_s3` reference example) |
| Rendering approach | u8g2 as graphics backend; output piped to ST7789 via 1bpp→RGB565 conversion each frame |
| Input | GPIO buttons only (KEY_0 = confirm, KEY_1 = back); touch input deferred |
| UI framework changes | Zero — `astra/ui/`, `astra/config/`, `astra/astra_logo.h`, and `hal/hal.h` are untouched |
| `astra_rocket.cpp` | **Not compiled** in ESP32 build (see Section 7). Replaced by `main/esp32_rocket.cpp`. |
| LVGL | **Not used.** esp_lcd handles ST7789 directly. |

## 3. Architecture

```
┌────────────────────────────────────────────────┐
│               ESP32-S3 (FreeRTOS)              │
│                                                │
│  ┌──────────────────────────────────────────┐  │
│  │              astra_task                  │  │
│  │  HAL::inject(new HAL_ESP32S3())          │  │
│  │  loop: Launcher::update()                │  │
│  │    → HAL::get()->draw*()  (u8g2)         │  │
│  │    → HAL::get()->canvasUpdate()          │  │
│  │        ↓ 1bpp→RGB565 convert             │  │
│  │        ↓ esp_lcd_panel_draw_bitmap()     │  │
│  └───────────────────────┬──────────────────┘  │
│                          │ SPI2                 │
│                          ▼                      │
│                 ST7789 240×320 TFT              │
└────────────────────────────────────────────────┘
```

**Unchanged:** `astra/` (UI framework), `hal/hal.h` (abstract interface).  
**New:** `HAL_ESP32S3` (concrete implementation), `driver/lcd_st7789`, `driver/key_gpio`, ESP-IDF build files.  
**Not used:** LVGL — u8g2 is the sole graphics backend; esp_lcd is the sole display transport.

## 4. Directory Structure

```
esp/
├── CMakeLists.txt                    # ESP-IDF project root
├── sdkconfig.defaults                # ESP32-S3 defaults (PSRAM, flash size)
├── partitions.csv                    # Flash partition table
│
└── main/
    ├── CMakeLists.txt                # Component definition + include paths
    ├── idf_component.yml             # Dependencies: esp_lcd only (no LVGL)
    ├── Kconfig.projbuild             # GPIO pin configuration (menuconfig)
    │
    ├── main.cpp                      # FreeRTOS entry, HAL init, task creation
    │
    ├── hal/
    │   ├── hal_esp32s3.h             # HAL_ESP32S3 class declaration
    │   └── hal_esp32s3.cpp           # All HAL method implementations
    │
    ├── driver/
    │   ├── lcd_st7789.h/c            # esp_lcd ST7789 init (SPI bus, panel handle)
    │   └── key_gpio.h/c              # GPIO button debounce state machine
    │
    ├── esp32_rocket.cpp              # Replaces astra_rocket.cpp — injects HAL_ESP32S3, builds menu tree
    └── astra/                        # Symlink or copy of sources/oled-ui-astra/Core/Src/astra/
                                      # astra_rocket.cpp and astra_rocket.h are NOT included
```

`hal/hal.h` is referenced via CMake include path pointing to `sources/oled-ui-astra/Core/Src/hal/hal.h` — not copied.

## 5. HAL_ESP32S3 Method Mapping

| HAL abstract method | ESP32-S3 implementation |
|---|---|
| `canvasUpdate()` | 1bpp→RGB565 convert (static buffer) + `esp_lcd_panel_draw_bitmap()` |
| `canvasClear()` | `u8g2_ClearBuffer()` |
| `drawPixel/Box/Line/RBox/BMP` | `u8g2_Draw*()` calls |
| `drawEnglish/Chinese`, `setFont` | `u8g2_SetFont()` + `u8g2_DrawUTF8()` |
| `getFontWidth` | `u8g2_GetUTF8Width()` (supports CJK — **not** `u8g2_GetStrWidth`) |
| `getFontHeight` | `u8g2_GetMaxCharHeight()` |
| `keyScan()` | Drives debounce logic in `key_gpio.c`; writes results into `HAL::key[]` and `HAL::keyFlag` |
| `getKey(i)` | Reads pre-populated `HAL::key[i]` (filled by `keyScan()`) |
| `delay(ms)` | `vTaskDelay(pdMS_TO_TICKS(ms))` |
| `millis()` | `esp_timer_get_time() / 1000` |
| `getTick()` | `xTaskGetTickCount()` |
| `beep(freq)` / `beepStop()` | `ledc_set_freq()` + `ledc_set_duty()` |
| `screenOn/Off()` | LEDC backlight channel duty 100% / 0% |
| `getSystemConfig()` | Returns logical canvas size: `screenWeight=128, screenHeight=64` (not physical TFT dimensions) |

> **Note on `getSystemConfig()`:** The field name is `screenWeight` (not `screenWidth`) — a typo in the original codebase that must **not** be corrected, as `launcher.cpp` references `HAL::getSystemConfig().screenWeight` directly. The logical size must remain 128×64; changing it to 240×320 would break all UI layout calculations.

## 6. u8g2 → ST7789 Pipeline

u8g2 is initialized with `u8g2_Setup_ssd1306_128x64_noname_f()` (the `_f` suffix allocates the full 1 KB internal framebuffer in RAM). The byte callback and gpio/delay callback are **no-op stubs** — they return 1 for all messages and handle only `U8X8_MSG_DELAY_MILLI` (via `vTaskDelay`). u8g2's SPI transport is **never used**; routing SSD1306 init commands to the ST7789 SPI bus would corrupt the panel.

All `u8g2_Draw*()` calls write directly to the in-RAM framebuffer. On each `_canvasUpdate()` call:

1. `u8g2_SendBuffer()` is called following standard u8g2 usage convention (the no-op byte callback means no SPI traffic occurs; this call is not strictly required but keeps u8g2 state consistent).
2. `u8g2_GetBufferPtr()` returns the 1 KB 1bpp framebuffer.
3. Convert 1bpp → RGB565: bit=1 → `0xFFFF` (white), bit=0 → `0x0000` (black). Result: 16 KB. `0xFFFF`/`0x0000` are byte-order-neutral, so endianness does not affect the black-and-white output (relevant if colors are added later: ST7789 expects big-endian RGB565 over SPI).
4. `esp_lcd_panel_draw_bitmap(panel, x0, y0, x0+128, y0+64, rgb565_buf)` transmits to ST7789.
5. Canvas appears in the top-left corner (or centered) of the 240×320 display.

> **Buffer allocation:** The 16 KB RGB565 conversion buffer must be declared as a `static` member of `HAL_ESP32S3` or a `static` local inside `_canvasUpdate()`. Stack allocation is forbidden — it would overflow `astra_task`'s stack.

Conversion is ~1 ms at 240 MHz — no impact on animation frame rate.

## 7. FreeRTOS Task Layout & Startup Sequence

Only one task is needed:

```c
// main.cpp
xTaskCreate(astra_task, "astra", 16384, NULL, 3, NULL);
```

### Why `astra_rocket.cpp` is NOT compiled on ESP32

`astraCoreInit()` (in `astra_rocket.cpp`) hardcodes `HAL::inject(new HALDreamCore)` and `astra_rocket.h` (C++ section, line 37) unconditionally includes `hal_dreamCore.h`, which pulls in STM32 peripheral headers (`spi.h`, `main.h`, etc.). Including either file in ESP32 code would break the build.

**Strategy:** Exclude `astra_rocket.cpp` from the ESP32 `idf_component_register(SRCS ...)`. Create `main/esp32_rocket.cpp` instead, which replicates only what is needed:

```cpp
// main/esp32_rocket.cpp — replaces astra_rocket.cpp for ESP32
#include "astra/ui/launcher.h"   // direct include, no hal_dreamCore
#include "hal/hal_esp32s3.h"
#include "astra/astra_logo.h"

static auto *astraLauncher = new astra::Launcher();
static auto *rootPage      = new astra::Tile("root");

void esp32_astra_init() {
    HAL::inject(new HAL_ESP32S3());   // inject ESP32 HAL first

    HAL::delay(350);
    astra::drawLogo(1000);

    // Build menu tree here (same as original astraCoreInit, minus HAL::inject)
    rootPage->addItem(new astra::List("test1"));
    // ...
    astraLauncher->init(rootPage);
}

void esp32_astra_start() {
    for (;;) {
        astraLauncher->update();
    }
}
```

`astra_task` flow:

```
esp32_astra_init()   ← injects HAL_ESP32S3, builds menu tree, shows logo
esp32_astra_start()  ← Launcher::update() loop
                       → HAL::keyScan() every frame (time-based debounce via millis())
                       → draw calls → canvasUpdate() → RGB565 → esp_lcd flush
```

No LVGL task. No LVGL dependency.

> **Stack sizing note:** 16 KB is the recommended starting point. Profile with `uxTaskGetHighWaterMark()` after first boot.

> **Known upstream behaviour:** `Launcher::update()` computes popup durations as `HAL::millis() / 1000` (launcher.cpp line 162), so the internal `time` counter is in **seconds**, not milliseconds. `popInfo("msg", 600)` will display for ~0.6 seconds, not 600 ms. This is pre-existing behaviour, not a port regression.

## 8. Build System

**`esp/CMakeLists.txt`:**
```cmake
cmake_minimum_required(VERSION 3.22)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(oled-ui-astra-esp32s3)
```

**`esp/main/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "main.cpp"
         "esp32_rocket.cpp"            # replaces astra_rocket.cpp (no STM32 includes)
         "hal/hal_esp32s3.cpp"
         "driver/lcd_st7789.c"         # compiled as C; headers need extern "C" guards
         "driver/key_gpio.c"           # compiled as C; headers need extern "C" guards
         # astra UI sources (do NOT include astra_rocket.cpp):
         "astra/ui/launcher.cpp"
         "astra/ui/item/item.cpp"
         "astra/ui/item/menu/menu.cpp"
         "astra/ui/item/selector/selector.cpp"
         "astra/ui/item/camera/camera.cpp"
         "astra/ui/item/widget/widget.cpp"
    INCLUDE_DIRS "."
                 "../../sources/oled-ui-astra/Core/Src"   # provides hal/hal.h and astra/
    REQUIRES esp_lcd driver ledc freertos esp_timer
)
```

> **Note:** `driver/lcd_st7789.c` and `driver/key_gpio.c` compile as C11. Their headers must use `extern "C" { }` guards so they can be included from C++ translation units (`main.cpp`, `hal_esp32s3.cpp`). `ledc` is listed explicitly because `screenOn/Off()` and `beep()` use LEDC; it is part of the `driver` umbrella but explicit listing avoids IDF version surprises.

**`esp/main/idf_component.yml`:**
```yaml
dependencies:
  idf: ">=5.1.0"
  # No LVGL dependency — u8g2 is bundled in sources/oled-ui-astra
```

**`esp/sdkconfig.defaults`:**
```ini
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
# Do not set CONFIG_IDF_TARGET here; set via: idf.py set-target esp32s3
```

## 9. Pin Configuration (Kconfig defaults)

| Signal | GPIO | Notes |
|---|---|---|
| LCD SCK | 18 | SPI2 CLK |
| LCD MOSI | 17 | SPI2 MOSI |
| LCD CS | 4 | SPI CS |
| LCD DC | 5 | Data/Command |
| LCD RST | 48 | Reset — **avoid GPIO3** (JTAG TDO on ESP32-S3-DevKitC-1) |
| LCD BL | 2 | LEDC backlight PWM |
| KEY_0 | 6 | Confirm (internal pull-up, active low) |
| KEY_1 | 7 | Back/navigate (internal pull-up, active low) |

All pins configurable via `idf.py menuconfig`.

## 10. Out of Scope (deferred)

- XPT2046 touch input
- Color/theme upgrade for UI
- WiFi / BLE connectivity
- NVS settings persistence
