# Hardware Audio I/O Integration Plan

**Date:** 2026-04-30
**Status:** Implementation in progress; this plan reflects the current WebSocket v1/raw-Opus design.
**Spec:** `docs/superpowers/specs/2026-04-30-hardware-audio-io-design.md`

## Goal

Integrate INMP441 microphone input, MAX98357A speaker output, physical wake button, WS2812 status LED, and XiaoZhi-compatible WebSocket assistant flow into `tft_touch_s3`.

The UI should remain a two-tab LVGL app. The AI tab renders `assistant_state`; audio and network work stays outside LVGL callbacks.

## Current Architecture

```text
button / AI tab
  -> assistant_start_listening()
  -> assistant task (WebSocket v1 JSON + raw Opus binary)
  -> audio_service queues and Opus codec tasks
  -> audio_io I2S RX/TX
  -> assistant_state polled by ui_page_ai.c
```

Key rules:

- WebSocket protocol target is Protocol-Version `1`.
- Control frames are JSON text.
- Audio frames are raw Opus binary frames.
- Do not use `proto_pack_audio()` or a 16-byte Binary Protocol 2 header on this path.
- WebSocket event callback payloads are transient; `audio_service_push_decode()` must copy payloads before enqueueing.
- Queue send failures must either free the producer-owned item or evict/free an older queued item according to `audio_service.c`.
- Session end, abort, disconnect, and error paths must stop input, mute output as needed, flush queues, and avoid old audio leaking into the next session.

## Pin Allocation

| Signal | GPIO | Notes |
| ------ | ---- | ----- |
| TFT SCLK | 18 | Existing SPI bus |
| TFT MOSI | 17 | Existing SPI bus |
| Touch MISO | 21 | Existing touch input |
| LCD backlight | 2 | Existing LEDC PWM |
| LCD RST/CS/DC | 3/4/5 | Existing display pins |
| Touch CS | 15 | Existing touch pin |
| Assistant button | 38 | Active-low, internal pull-up |
| WS2812 data | 48 | RMT output |
| INMP441 BCLK/WS/DIN | 8/9/10 | I2S RX |
| MAX98357A BCLK/LRCLK/DOUT/SD_MODE | 11/12/13/14 | I2S TX and mute |

Do not move assistant defaults onto TFT/touch pins. Avoid GPIO19/GPIO20 for this demo because they are commonly used by native USB on ESP32-S3 boards.

## Implementation Checklist

- [x] Add HAL, audio, assistant, and UI source files to `main/CMakeLists.txt`.
- [x] Add managed dependencies for `esp_audio_codec`, `button`, `led_strip`, `esp_websocket_client`, and cJSON.
- [x] Add `Assistant Hardware` Kconfig values with non-conflicting defaults.
- [x] Add `btn` and `led_status` HAL modules.
- [x] Add `audio_io` I2S RX/TX module.
- [x] Add `audio_service` tasks, Opus encode/decode, queue flushing, and bounded backpressure.
- [x] Add `assistant_state` guarded by a FreeRTOS mutex.
- [x] Add WebSocket v1 JSON helpers in `assistant_proto`.
- [x] Add `assistant` WebSocket task with Protocol-Version, Device-Id, Client-Id, and optional Authorization headers.
- [x] Wire AI page polling to `assistant_state`.
- [x] Wire `main.c` initialization before LCD/LVGL startup.
- [x] Confirm includes use `#include "cJSON.h"` with `espressif__cjson` in `PRIV_REQUIRES`.
- [ ] Re-run `idf.py build` after the cJSON include correction.
- [ ] Flash hardware and verify button, LED, microphone upload, TTS playback, and UI state transitions.

## Build Notes

Use:

```powershell
idf.py set-target esp32s3
idf.py build
```

If build output reports old assistant GPIO defaults such as `35`-`42`, those values are coming from local `sdkconfig`; update them with `idf.py menuconfig` or regenerate local config. Do not commit `sdkconfig` unless explicitly requested.

If cJSON headers fail, expected code state is:

- `main/CMakeLists.txt` contains `espressif__cjson` in `PRIV_REQUIRES`.
- `assistant.c` and `assistant_proto.c` include `#include "cJSON.h"`.

## Verification Plan

1. `idf.py build` completes and reports the app fits in the 3 MB partition.
2. Boot log shows `audio_io`, `btn`, and `led_status` initialization.
3. Idle LED shows blue.
4. Pressing the wake button starts WebSocket connection and sends hello.
5. Server hello sets session and audio parameters.
6. Mic Opus frames are sent as raw binary WebSocket frames.
7. TTS binary frames are decoded and played through MAX98357A.
8. AI tab transitions through listening, uploading, thinking, speaking, and idle/error from `assistant_state`.
