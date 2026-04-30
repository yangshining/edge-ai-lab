# AGENTS.md

This file gives Codex guidance for working inside the `tft_touch_s3` ESP-IDF example.

## Project Scope

This directory is an ESP32-S3 Edge AI Lab demo. It drives:

- ST7789 SPI TFT display, 240x320
- XPT2046 resistive touch controller
- LVGL 9.3 two-tab AI assistant UI (`AI` and `Setup`)
- BLE WiFi provisioning through `espressif/network_provisioning`
- WiFi STA status display and credential clearing on the Setup page
- INMP441/MAX98357A audio path, wake button, WS2812 status LED, and XiaoZhi WebSocket v1 assistant flow

Keep changes focused on this example unless the user explicitly asks for framework-level ESP-IDF changes.

## Key Files

- `main/main.c` - app entry point, NVS setup, connectivity startup, LVGL task/tick setup
- `main/lcd_touch.c` and `main/lcd_touch.h` - LCD, touch, SPI, backlight PWM, and LVGL lock integration
- `main/sys_stats.c` and `main/sys_stats.h` - heap and CPU load sampling
- `main/settings_store.c` and `main/settings_store.h` - NVS-backed brightness and rotation settings
- `main/connectivity/app_net_state.c` and `.h` - shared provisioning/WiFi state model guarded by a FreeRTOS mutex
- `main/connectivity/app_wifi.c` and `.h` - WiFi STA, `esp_netif`, WiFi/IP event handling, credential clearing
- `main/connectivity/app_prov.c` and `.h` - BLE provisioning manager integration
- `main/hal/btn.c` and `.h` - `espressif/button` wrapper for the active-low assistant wake button
- `main/hal/led_status.c` and `.h` - WS2812 status LED via `espressif/led_strip`
- `main/audio/audio_io.c` and `.h` - I2S RX/TX setup for INMP441 and MAX98357A
- `main/audio/audio_service.c` and `.h` - audio tasks, Opus encode/decode, queue ownership, flush APIs, and backpressure
- `main/assistant/assistant_state.c` and `.h` - assistant state model guarded by a FreeRTOS mutex
- `main/assistant/assistant_proto.c` and `.h` - XiaoZhi WebSocket v1 JSON helpers; audio frames are raw Opus binary
- `main/assistant/assistant.c` and `.h` - lazy-start WebSocket assistant task, headers, session lifecycle, and audio queue handoff
- `main/ui/ui_config.h` - central UI constants (timer periods per avatar state, animation parameters, widget dimensions); add new UI constants here instead of inline magic numbers
- `main/ui/ui_main.c` - LVGL two-tab tabview creation and page wiring
- `main/ui/ui_page_ai.c` - voice-assistant avatar page; polls `assistant_state` and maps assistant states to avatar states
- `main/ui/ui_page_settings.c` - rotation, backlight, WiFi status, and Clear WiFi controls
- `main/Kconfig.projbuild` - GPIO, touch, provisioning, and WiFi retry configuration
- `main/idf_component.yml` - managed dependencies
- `partitions.csv` - custom 3 MB factory app partition

## Build And Verification

ESP-IDF must be activated before building.

```powershell
idf.py set-target esp32s3
idf.py build
```

For hardware verification:

```powershell
idf.py -p COMx flash monitor
```

If build fails with a missing WiFi binary library such as `components/esp_wifi/lib/esp32s3/libcore.a`, initialize the ESP-IDF WiFi binary submodule:

```powershell
git submodule update --init --recursive -- components/esp_wifi/lib
```

Do not commit `build/`, `sdkconfig`, `sdkconfig.old`, downloaded `managed_components/`, or submodule SHA changes unless the user explicitly asks.

## Connectivity Rules

- `nvs_flash_init()` must run before settings, WiFi, or provisioning code.
- BLE provisioning defaults are controlled by Kconfig:
  - `CONFIG_EXAMPLE_PROV_SERVICE_NAME="edge-ai-lab"`
  - `CONFIG_EXAMPLE_PROV_POP="abcd1234"`
  - `CONFIG_EXAMPLE_WIFI_MAX_RETRY=5`
- WiFi/BLE event handlers must not call LVGL directly.
- Event handlers should update `app_net_state`; LVGL pages should read that state from LVGL task context.
- The Setup page currently uses an LVGL timer to refresh status labels from `app_net_state`.
- The **Clear WiFi** button calls provisioning reset/credential clearing and restarts the board.
- Assistant networking/audio work must stay out of LVGL callbacks. UI code should call `assistant_start_listening()` and poll `assistant_state` from LVGL task context.
- The assistant WebSocket path targets Protocol-Version `1`: JSON text frames plus raw Opus binary frames. Do not add the 16-byte Binary Protocol 2 header unless a separate v2 transport is designed.
- WebSocket request headers are built in `assistant.c`: `Protocol-Version`, `Device-Id`, persisted `Client-Id`, and optional `Authorization`.
- Audio queues pass heap-owned pointers. On enqueue failure, the producer or helper must free or evict according to `audio_service.c`; do not enqueue transient WebSocket callback buffers directly.

## LVGL Threading

LVGL runs in the dedicated `lvgl_task`. Any LVGL call from another FreeRTOS task must be guarded with the shared lock declared in `lcd_touch.h`:

```c
_lock_acquire(&lvgl_api_lock);
/* LVGL calls */
_lock_release(&lvgl_api_lock);
```

The AI page API `ui_ai_update_result()` must be called under this lock if invoked outside LVGL task context.

## Configuration Notes

- GPIO defaults should stay consistent across `README.md`, `Kconfig.projbuild`, and wiring assumptions.
- Assistant GPIO defaults are button `38`, LED `48`, mic `8/9/10`, speaker `11/12/13/14`; keep them away from TFT/touch pins and GPIO19/20.
- A local `sdkconfig` overrides changed Kconfig defaults. Do not commit `sdkconfig` just to update local assistant GPIOs unless explicitly requested.
- `sdkconfig.defaults.esp32s3` enables PSRAM, Bluetooth, NimBLE, and FreeRTOS runtime stats.
- `sdkconfig.defaults` enables LVGL defaults, XPT2046 defaults, the custom partition table, and protocomm security v1.
- The current app binary needs the custom 3 MB factory partition because LVGL plus BLE/WiFi/audio assistant dependencies are larger than the stock small app partition.

## Documentation Rules

When changing this example:

- Update `README.md` for user-facing behavior, build steps, hardware notes, and troubleshooting.
- Update this `AGENTS.md` and `CLAUDE.md` when module responsibilities or workflow rules change.
- Keep connectivity architecture documented: event handlers update state, UI renders state.
