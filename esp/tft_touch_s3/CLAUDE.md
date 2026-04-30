# CLAUDE.md

This file provides guidance to Claude Code when working in this ESP-IDF example directory.

## Project Overview

This is the `tft_touch_s3` ESP32-S3 example inside the ESP-IDF repository. It drives an ST7789 SPI TFT display with an XPT2046 resistive touch controller using LVGL 9.3.

The current demo is an Edge AI Lab screen with a two-tab AI assistant UI:

- **AI** - voice-assistant avatar driven by `assistant_state`, with physical wake button, I2S audio, RGB LED status, and XiaoZhi WebSocket v1 session flow
- **Setup** - rotation, backlight brightness, WiFi STA status, and WiFi credential clearing controls

External component dependencies are declared in `main/idf_component.yml`:

- `lvgl/lvgl: "9.3.0"`
- `atanisoft/esp_lcd_touch_xpt2046: "1.0.6"`
- `espressif/network_provisioning: "^1.2.4"`
- `espressif/esp_audio_codec: "~2.4.1"`
- `espressif/button: "~4.1.5"`
- `espressif/led_strip: "~3.0.2"`
- `espressif/esp_websocket_client: "~1.1.0"`

## Build Commands

Run commands from this directory with ESP-IDF activated.

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
idf.py menuconfig
```

Use the `TFT Touch S3 Example Configuration` menu for GPIO, touch, BLE provisioning, and WiFi retry settings.

If `components/esp_wifi/lib/esp32s3/libcore.a` is missing, initialize the WiFi binary submodule from the repository root:

```powershell
git submodule update --init --recursive -- components/esp_wifi/lib
```

Do not commit `build/`, local `sdkconfig`, `sdkconfig.old`, managed component downloads, or submodule SHA changes unless explicitly requested.

## Architecture

```text
app_main (main.c)
  |-- nvs_flash_init()
  |-- settings_store_init()       -> settings_store.c
  |-- sys_stats_init()            -> sys_stats.c
  |-- app_net_state_init()        -> connectivity/app_net_state.c
  |-- app_wifi_init()             -> connectivity/app_wifi.c
  |-- app_prov_init()             -> connectivity/app_prov.c
  |-- assistant_state_init()      -> assistant/assistant_state.c
  |-- audio_io_init()             -> audio/audio_io.c
  |-- audio_service_init()        -> audio/audio_service.c
  |-- btn_init()                  -> hal/btn.c
  |-- led_status_init()           -> hal/led_status.c
  |-- assistant_init()            -> assistant/assistant.c
  |-- lcd_touch_init()            -> lcd_touch.c / lcd_touch.h
  |-- LVGL display/input/task     -> main.c
  `-- ui_main_init()              -> ui/ui_main.c
        |-- ui_page_ai_init()
        `-- ui_page_settings_init()
```

## Connectivity Model

BLE provisioning uses the `network_provisioning` managed component with BLE transport and protocomm security v1.

Defaults:

```text
CONFIG_EXAMPLE_PROV_SERVICE_NAME="edge-ai-lab"
CONFIG_EXAMPLE_PROV_POP="abcd1234"
CONFIG_EXAMPLE_WIFI_MAX_RETRY=5
```

Rules:

- `nvs_flash_init()` must happen before WiFi/provisioning initialization.
- WiFi/BLE event handlers update `app_net_state`.
- Event handlers must not call LVGL directly.
- The Setup page uses an LVGL timer to read `app_net_state` and refresh labels.
- Clearing WiFi credentials restarts the board so provisioning begins again.

## LVGL Threading

LVGL runs in a dedicated FreeRTOS task (`lvgl_task`, priority 2, 6 KB stack). Any LVGL API call from outside LVGL task context must be protected by the shared lock declared in `lcd_touch.h`:

```c
_lock_acquire(&lvgl_api_lock);
/* LVGL calls here */
_lock_release(&lvgl_api_lock);
```

`lvgl_api_lock` is defined in `lcd_touch.c` and initialized by `lcd_touch_init()`.

## AI Assistant

The AI tab polls `assistant_state` from LVGL task context and maps assistant states to avatar states. The on-screen wake button and physical active-low button both call `assistant_start_listening()`, which lazily creates the WebSocket assistant task.

`ui_ai_update_result(const char *label, float confidence)` remains as a compatibility hook. The label updates the avatar caption/speaking state, while confidence is ignored. Call it under `lvgl_api_lock` when invoked from outside LVGL task context.

Keep network and audio work out of LVGL callbacks; UI files should only render state on the LVGL task side.

Assistant protocol rules:

- Target XiaoZhi WebSocket Protocol-Version `1`.
- Control messages are JSON text frames.
- Audio messages are raw Opus WebSocket binary frames; do not prepend a Binary Protocol 2 header on this path.
- `assistant.c` sets `Protocol-Version`, `Device-Id`, persisted `Client-Id`, and optional `Authorization` headers.
- `assistant_proto.c` should stay limited to JSON builders/parsers unless a separate v2 transport is added.

Audio ownership rules:

- `audio_service.c` queues pass heap-owned `audio_pcm_frame_t *` or `audio_opus_pkt_t *`.
- `audio_service_push_decode()` copies WebSocket payloads before enqueueing; never queue a transient callback buffer pointer.
- On full queues, follow the local drop/free policy in `audio_service.c` and keep audio latency bounded.

## Settings And Stats

- `settings_store.c/.h` persists brightness and rotation in NVS namespace `"tft_settings"`.
- `sys_stats.c/.h` samples heap and CPU load using `esp_timer` and `uxTaskGetSystemState()`.
- Backlight is controlled by LEDC PWM through `lcd_touch_set_brightness(uint8_t pct)`.
- Assistant hardware defaults: button `38`, LED `48`, mic BCLK/WS/DIN `8/9/10`, speaker BCLK/LRCLK/DOUT/SD_MODE `11/12/13/14`.

## Configuration

Important defaults:

- `sdkconfig.defaults.esp32s3` enables PSRAM, Bluetooth, NimBLE, and FreeRTOS runtime stats.
- `sdkconfig.defaults` enables LVGL defaults, XPT2046 defaults, custom partition table, and protocomm security v1.
- `partitions.csv` gives the factory app a 3 MB partition for LVGL plus BLE/WiFi.

UI layout constants (timer periods, widget sizes, animation parameters) belong in `main/ui/ui_config.h`. Do not use inline magic numbers in UI page files.

Keep `README.md`, `AGENTS.md`, `CLAUDE.md`, `Kconfig.projbuild`, and GPIO defaults consistent when changing behavior. A local `sdkconfig` can preserve old GPIO values after Kconfig defaults change; update it locally through `idf.py menuconfig` but do not commit it unless explicitly requested.
