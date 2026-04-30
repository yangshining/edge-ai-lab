| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

# TFT Touch S3 - Edge AI Lab Demo

This example drives a 2.4-inch SPI TFT module marked `TFT SPI 240*320`, with an SPI resistive touch controller. The display path uses the ESP-IDF `esp_lcd` ST7789 driver and the touch path uses the XPT2046 component.

The default configuration has been verified on an ESP32-S3 N16R8 development board with a 2.4-inch 240x320 SPI TFT module. The verified setup uses active-high backlight control, XPT2046 polling mode, mirrored X touch coordinates, and non-mirrored Y touch coordinates.

The demo renders a compact two-tab LVGL v9 UI for the small 240x320 touch screen:

- **AI:** voice-assistant avatar wired to the local assistant state, physical wake button, I2S microphone/speaker path, RGB status LED, and XiaoZhi-compatible WebSocket v1 control/audio flow.
- **Setup:** screen rotation, backlight brightness slider, WiFi STA status, and **Clear WiFi** credential reset. Rotation and brightness are persisted to NVS and restored on reboot.

## Hardware

- ESP32-S3 development board, such as the XinluCity ESP32-S3 N16R8 board
- 2.4-inch SPI TFT 240x320 module
- XPT2046-compatible resistive touch controller
- INMP441 I2S microphone
- MAX98357A I2S amplifier and speaker
- Active-low wake button
- WS2812-compatible RGB LED
- USB cable for flashing

The photographed TFT module has these pins:

```text
VCC, GND, CS, RESET, DC, SDI<MOSI>, SCK, LED, SDO<MISO>,
T_CLK, T_CS, T_DIN, T_DO, T_IRQ,
SD_CS, SD_MOSI, SD_MISO, SD_SCK
```

The `SD_*` pins are for the TF card slot. This example does not use the TF card, so leave them unconnected.

## Verified Behavior

- The TFT backlight is driven by `GPIO2` via **LEDC PWM** (5 kHz, 13-bit) and is active high by default. Call `lcd_touch_set_brightness(pct)` to change brightness programmatically.
- The UI uses English labels so it works with the built-in Montserrat font and does not need a CJK font.
- LVGL's performance overlay is disabled by default.
- XPT2046 touch input is tuned for responsiveness with one sample per read and a pressure threshold of `50`.
- Touch calibration defaults are `swap_xy = n`, `mirror_x = y`, and `mirror_y = n`.
- Brightness and rotation settings are saved to NVS partition `"tft_settings"` and restored on every boot.
- The Setup page uses BLE provisioning for first-time WiFi setup, then shows WiFi STA connection state from a small shared state model.
- The AI page polls `assistant_state`; the physical button or on-screen button starts the assistant session.
- The assistant WebSocket uses Protocol-Version `1`: JSON text control frames plus raw Opus binary audio frames. It does not prepend a Binary Protocol 2 header.

## Software Structure

The demo keeps display, UI, and connectivity separated:

- `main/main.c` initializes NVS, settings, runtime stats, connectivity, LCD/touch, LVGL, and the UI.
- `main/lcd_touch.c` owns the ST7789 panel, XPT2046 touch controller, backlight PWM, and exported LVGL API lock.
- `main/connectivity/app_net_state.c` stores the current provisioning/WiFi state behind a FreeRTOS mutex.
- `main/connectivity/app_wifi.c` initializes `esp_netif`/WiFi STA and translates WiFi/IP events into `app_net_state` updates.
- `main/connectivity/app_prov.c` starts BLE provisioning through the `network_provisioning` managed component and handles provisioning events.
- `main/hal/btn.c` wraps `espressif/button` for the active-low assistant wake button.
- `main/hal/led_status.c` drives a single WS2812 status LED through RMT.
- `main/audio/audio_io.c` owns I2S RX/TX setup for INMP441 and MAX98357A.
- `main/audio/audio_service.c` owns the audio FreeRTOS tasks, Opus encode/decode, queue ownership, flushing, and backpressure behavior.
- `main/assistant/assistant_state.c` stores cross-task assistant status behind a FreeRTOS mutex.
- `main/assistant/assistant_proto.c` builds/parses XiaoZhi WebSocket v1 JSON messages; audio packets are raw Opus binary frames.
- `main/assistant/assistant.c` owns the lazy-start WebSocket task, request headers, session lifecycle, and audio queue handoff.
- `main/ui/ui_main.c` creates the LVGL tabview and wires the AI and Setup pages.
- `main/ui/ui_page_ai.c` renders the voice-assistant avatar by polling `assistant_state`.
- `main/ui/ui_page_settings.c` renders rotation/backlight controls and refreshes WiFi labels from `app_net_state`; WiFi/BLE event handlers do not call LVGL directly.

## Recommended Wiring

Use the UART/CH340 USB port on the ESP32-S3 board for flashing. Avoid GPIO19/GPIO20 for this demo because they are also the native USB D-/D+ pins on many ESP32-S3 boards.

| TFT module pin | ESP32-S3 pin | Notes |
| -------------- | ------------ | ----- |
| `VCC` | `3V3` | Use 3.3 V first; ESP32-S3 GPIOs are not 5 V tolerant |
| `GND` | `GND` | Common ground |
| `CS` | `GPIO4` | LCD chip select |
| `RESET` | `GPIO3` | LCD reset |
| `DC` | `GPIO5` | LCD data/command |
| `SDI<MOSI>` | `GPIO17` | Shared SPI MOSI |
| `SCK` | `GPIO18` | Shared SPI clock |
| `LED` | `GPIO2` | Backlight control, active high by default |
| `SDO<MISO>` | optional, or `GPIO21` | LCD readback is not required |
| `T_CLK` | `GPIO18` | Touch SPI clock, shared with LCD |
| `T_CS` | `GPIO15` | Touch chip select |
| `T_DIN` | `GPIO17` | Touch SPI MOSI, shared with LCD |
| `T_DO` | `GPIO21` | Touch SPI MISO; required for coordinates |
| `T_IRQ` | unconnected | Current code polls touch data and does not use interrupt |

Assistant hardware defaults avoid the TFT/touch pins and the common ESP32-S3 native USB pins:

| Assistant signal | ESP32-S3 pin | Notes |
| ---------------- | ------------ | ----- |
| Wake button | `GPIO38` | Active-low, internal pull-up |
| WS2812 RGB LED data | `GPIO48` | Board-specific if an onboard RGB LED uses another pin |
| INMP441 BCLK | `GPIO8` | I2S RX |
| INMP441 WS | `GPIO9` | I2S RX word select |
| INMP441 DIN | `GPIO10` | I2S RX data input |
| MAX98357A BCLK | `GPIO11` | I2S TX |
| MAX98357A LRCLK | `GPIO12` | I2S TX word select |
| MAX98357A DIN | `GPIO13` | I2S TX data output from ESP32-S3 |
| MAX98357A SD_MODE | `GPIO14` | High enables output; low mutes |

Default Kconfig values match these tables. To change pins, run:

```bash
idf.py menuconfig
```

Then open `Example Configuration`.

## Build

From a PowerShell session:

```powershell
cd E:\code\esp\esp-idf
. .\export.ps1
cd examples\peripherals\lcd\tft_touch_s3
idf.py set-target esp32s3
idf.py build
```

From a Unix-like shell:

```bash
cd /path/to/esp-idf
. ./export.sh
cd examples/peripherals/lcd/tft_touch_s3
idf.py set-target esp32s3
idf.py build
```

For day-to-day UI/code changes, use incremental builds:

```powershell
idf.py build
idf.py -p COMx app-flash monitor
```

Use `idf.py fullclean` only after target changes, major Kconfig changes, managed component problems, or a corrupted build directory.
After pulling the BLE provisioning changes, run `idf.py set-target esp32s3` or `idf.py reconfigure` once so the Bluetooth and custom partition defaults are applied to the local `sdkconfig`.
After pulling GPIO default changes, note that a local `sdkconfig` keeps its old values. Run `idf.py menuconfig` to update pins, or regenerate local config if you want to adopt the new Kconfig defaults.
If the build fails with a missing file under `components/esp_wifi/lib/esp32s3`, initialize the ESP-IDF submodules:

```powershell
git submodule update --init --recursive -- components/esp_wifi/lib
```

The build output is written under:

```text
examples/peripherals/lcd/tft_touch_s3/build
```

## Flash and Monitor

Replace `COMx` with the serial port shown in Device Manager, for example `COM3` or `COM7`.

```powershell
idf.py -p COMx flash monitor
```

You can also flash first and open the monitor later:

```powershell
idf.py -p COMx flash
idf.py -p COMx monitor
```

Exit the monitor with `Ctrl+]`.

Expected log lines include:

```text
I (...) lcd_touch: Init XPT2046 touch
I (...) lcd_touch: LCD and touch initialised
I (...) main: LVGL task started
I (...) app_prov: Starting BLE provisioning
```

## Configuration Notes

The current defaults are:

| Config option | Default |
| ------------- | ------- |
| `EXAMPLE_PIN_NUM_SCLK` | `18` |
| `EXAMPLE_PIN_NUM_MOSI` | `17` |
| `EXAMPLE_PIN_NUM_MISO` | `21` |
| `EXAMPLE_PIN_NUM_LCD_CS` | `4` |
| `EXAMPLE_PIN_NUM_LCD_DC` | `5` |
| `EXAMPLE_PIN_NUM_LCD_RST` | `3` |
| `EXAMPLE_PIN_NUM_TOUCH_CS` | `15` |
| `EXAMPLE_PIN_NUM_BK_LIGHT` | `2` |
| `EXAMPLE_BK_LIGHT_ON_LEVEL` | `1` |
| `EXAMPLE_TOUCH_SWAP_XY` | `n` |
| `EXAMPLE_TOUCH_MIRROR_X` | `y` |
| `EXAMPLE_TOUCH_MIRROR_Y` | `n` |
| `EXAMPLE_TOUCH_LOG` | `n` |
| `EXAMPLE_PROV_SERVICE_NAME` | `edge-ai-lab` |
| `EXAMPLE_PROV_POP` | `abcd1234` |
| `EXAMPLE_WIFI_MAX_RETRY` | `5` |
| `EXAMPLE_LCD_PIXEL_CLOCK_HZ` | `20000000` |
| `XPT2046_Z_THRESHOLD` | `50` |
| `ESP_LCD_TOUCH_MAX_POINTS` | `1` |
| `ASSISTANT_BTN_GPIO` | `38` |
| `ASSISTANT_LED_GPIO` | `48` |
| `ASSISTANT_MIC_BCLK` | `8` |
| `ASSISTANT_MIC_WS` | `9` |
| `ASSISTANT_MIC_DIN` | `10` |
| `ASSISTANT_SPK_BCLK` | `11` |
| `ASSISTANT_SPK_LRCLK` | `12` |
| `ASSISTANT_SPK_DOUT` | `13` |
| `ASSISTANT_SPK_SD_MODE` | `14` |
| `ASSISTANT_WS_URL` | `wss://api.xiaozhi.me/xiaozhi/v1/` |
| `ASSISTANT_WS_TOKEN` | empty |

`sdkconfig.defaults.esp32s3` enables octal PSRAM at 80 MHz and FreeRTOS runtime stats:

```text
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER=y
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1=y
```

This matches ESP32-S3 N16R8 boards with 8 MB PSRAM. If your board has no compatible PSRAM, remove or override the `CONFIG_SPIRAM*` lines. The `FREERTOS_*` lines support the optional legacy stats page and are safe to keep regardless of PSRAM. The Bluetooth lines enable the BLE transport used by WiFi provisioning.

## BLE WiFi Provisioning

On first boot, or after clearing WiFi credentials from the Setup page, the device starts BLE provisioning using the `network_provisioning` managed component. Use Espressif's provisioning app or tooling with:

- Transport: BLE
- Service name: `edge-ai-lab-XXXXXX`, where `XXXXXX` is derived from the STA MAC address
- Proof-of-possession: `abcd1234` by default

After provisioning succeeds, the device connects as a WiFi station and the Setup page shows the connected SSID and IPv4 address. The **Clear WiFi** button erases stored WiFi credentials and restarts the board so it enters provisioning again.

The provisioning and WiFi event handlers only update `app_net_state`. LVGL labels are updated by an LVGL timer in the Setup page, keeping UI work on the LVGL task side.

## Integrating AI Results

The AI page is driven by `assistant_state`. A press on the on-screen wake button or the physical wake button calls `assistant_start_listening()`, which lazily creates the WebSocket assistant task. The assistant task sends XiaoZhi WebSocket v1 JSON control frames, sends microphone Opus packets as raw binary frames, and pushes raw Opus response frames into the decode queue.

The WebSocket handshake sets these headers:

- `Protocol-Version: 1`
- `Device-Id`: lowercase STA MAC address
- `Client-Id`: generated once and persisted in NVS namespace `"assistant"`, key `"client_id"`
- `Authorization: Bearer <token>` when `ASSISTANT_WS_TOKEN` is configured

`ui_ai_update_result(const char *label, float confidence)` remains as a compatibility hook. The label updates the avatar caption and speaking state, while confidence is ignored. If it is invoked from outside the LVGL task context, call it while holding `lvgl_api_lock`:

```c
#include "ui_page_ai.h"
#include "lcd_touch.h"

_lock_acquire(&lvgl_api_lock);
ui_ai_update_result("Here is a reply", 0.0f);
_lock_release(&lvgl_api_lock);
```

Keep audio and network work out of LVGL callbacks. LVGL-side timers/pages should render assistant state from LVGL task context.

## Troubleshooting

- Blank white screen: the module may use ILI9341 instead of ST7789, or the SPI pins may be wrong.
- Screen is black: check `LED -> GPIO2` backlight wiring. If the module uses active-low backlight, set `EXAMPLE_BK_LIGHT_ON_LEVEL` to `0` in `idf.py menuconfig`.
- No touch response: check `T_DO -> GPIO21`, `T_DIN -> GPIO17`, `T_CLK -> GPIO18`, and `T_CS -> GPIO15`.
- Enable `EXAMPLE_TOUCH_LOG` in `idf.py menuconfig` to print `touch: x=... y=... z=...` while calibrating. Keep it disabled for smoother touch response.
- Touch is mirrored or offset: adjust `EXAMPLE_TOUCH_SWAP_XY`, `EXAMPLE_TOUCH_MIRROR_X`, and `EXAMPLE_TOUCH_MIRROR_Y` in `idf.py menuconfig`.
- Touch is sluggish: keep `EXAMPLE_TOUCH_LOG` disabled, keep `ESP_LCD_TOUCH_MAX_POINTS=1`, and lower `XPT2046_Z_THRESHOLD` carefully if light touches are missed.
- Build fails because PSRAM is not found: adjust `sdkconfig.defaults.esp32s3` for your board.
- Build fails with `components/esp_wifi/lib/esp32s3/libcore.a` missing: initialize the `components/esp_wifi/lib` submodule.
- Build still shows old assistant GPIO defaults such as `35`-`42`: those values are coming from local `sdkconfig`, not `Kconfig.projbuild`. Update them in `idf.py menuconfig` or regenerate local config.
- Build fails with `cJSON.h` missing: confirm `main/CMakeLists.txt` has `espressif__cjson` in `PRIV_REQUIRES` and includes use `#include "cJSON.h"`.
- Assistant connects but audio is garbled: confirm the server endpoint expects Protocol-Version `1` with raw Opus binary frames. Do not use a 16-byte Binary Protocol 2 header on this path.
- The Setup page still waits for provisioning after entering credentials: verify the POP (`abcd1234` by default), confirm the AP is 2.4 GHz, and check serial logs for `NETWORK_PROV_WIFI_CRED_FAIL`.
- Do not connect the TF card `SD_*` pins unless SD card support is added to the example.
