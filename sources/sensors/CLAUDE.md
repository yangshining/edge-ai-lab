# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

STM32F103C8T6-based IoT environmental monitoring system. Reads sensors, displays data on OLED, uploads to OneNET cloud via MQTT/ESP8266, controls actuators based on configurable thresholds.

## Build System

**Keil MDK-ARM V5** project — no Makefile or CMake.

- Open `stm32f103.uvprojx`, build with F7
- Output: `output/stm32f103.hex` / `.bin` / `.elf`
- Compiler: ARM Compiler 5.06 (-O1)
- Flash via ST-Link; debug output on UART1 at 115200 baud (PA9/PA10)

## Architecture

```
user/main.c          → main loop: Refresh_Sensors / Update_Actuators / Refresh_Display
hardware/            → sensor + actuator drivers
  adc.c / adc.h      → DMA 5-channel ADC (use ADC_IDX_* macros, never raw indices)
  mq.c               → MQ gas sensors (MQ5/MQ6 stub — PA2/PA3 taken by USART2)
  dht11.c            → DHT11 single-wire
  fan.c / beep.c     → actuator GPIO control
NET/
  device/esp8266.c   → ESP8266 AT-command driver (USART2, volatile buf)
  MQTT/MqttKit.c     → MQTT packet implementation
  onenet/onenet.c    → OneNET auth (HMAC-SHA1/Base64) + publish/subscribe
  CJSON/cJSON.c      → JSON parsing for cloud commands
OLED/oled.c          → I2C OLED driver (SDA=PB0, SCL=PB1)
core/                → CMSIS, startup, interrupt handlers
fwlib/               → STM32 Standard Peripheral Library — do not modify
```

**Main loop flow:** Init → WiFi → MQTT connect → OneNET auth → loop every 2ms:
- Every 200 ticks (~5s): read sensors, upload JSON, clear ESP buffer
- Every tick: receive cloud commands, run actuator logic, refresh OLED

## Pin Mapping

| Signal | Pin | Notes |
|--------|-----|-------|
| DHT11 | PA11 | Single-wire |
| MQ3 / MQ4 | PA0 / PA1 | ADC ch0/1 → `ADC_IDX_MQ3/4` |
| Light / MQ2 / MQ7 | PA4 / PA5 / PA6 | ADC ch4/5/6 → `ADC_IDX_LIGHT/MQ2/MQ7` |
| MQ5 / MQ6 | PA2 / PA3 | **NOT available** — USART2 TX/RX (ESP8266) |
| Fan | PA8 | GPIO out |
| Buzzer / LED / Irrigation | PC13 / PC14 / PC15 | GPIO out |
| ESP8266 USART2 | PA2(TX) / PA3(RX) | 115200 baud |
| Debug UART1 | PA9(TX) / PA10(RX) | 115200 baud |
| OLED SCL / SDA | PB1 / PB0 | I2C |

## Key Configuration

Thresholds (`user/main.c`, also settable from cloud):
```c
u8  temp_adj = 37;   // Temperature alarm (°C)
u8  humi_adj = 90;   // Humidity alarm (%)
int smog_th  = 50;   // MQ2 smoke alarm (%)
int light_th = 30;   // Light threshold for LED (%)
```

Credentials — must be changed before deploy:
- OneNET: `PROID`, `ACCESS_KEY`, `DEVICE_NAME` in `NET/onenet/src/onenet.c:51-55`
- WiFi: `ESP8266_WIFI_INFO` in `NET/device/src/esp8266.c:36`

## ADC / DMA Rules

`NOFCHANEL = 5`. DMA fills `ADC_ConvertedValue[]` in rank order. Always use the `ADC_IDX_*` macros defined in `adc.h` — never raw indices. The rank in `ADC_RegularChannelConfig` is always `ADC_IDX_* + 1`.

`RCC_ADCCLKConfig` **must** be called before `ADC_Init` (ADC clock max 14 MHz; default after reset is 36 MHz which violates spec).

## ISR / Volatile Rules

`esp8266_buf`, `esp8266_cnt`, `esp8266_cntPre` are written in `USART2_IRQHandler` — declared and extern'd as `volatile`. Before passing to standard library functions (`strstr`, `memset`), always copy to a local non-volatile buffer first (`memcpy` snapshot in `ESP8266_SendCmd`; byte loop in `ESP8266_Clear`).

## OneNET Payload

Uploaded datapoints (field names match OneNET product template):

| Field | Type | Source |
|-------|------|--------|
| `temp` | int | DHT11 temperature |
| `humi` | int | DHT11 humidity |
| `light` | int | PA4 light sensor, 0–99 |
| `beep` | bool | Buzzer state |

Cloud can set: `fan`, `led`, `temp_adj`, `humi_adj`, `light_th` via `$sys/{PROID}/{DEVICE_NAME}/thing/property/set`.

> **Migration notes:**
> - Field `light` was previously named `smog1` (incorrect).
> - Writable property `light_th` was previously named `fan_adj`. If the OneNET product template is not updated, the device will silently ignore cloud commands for this property and the LED grow-light threshold will be stuck at the firmware default (30%). Update the property name in the OneNET console before deploying.

## Actuator Override Logic

`fan_mode` / `led_mode` flags: `1` = auto (threshold-driven), `3` = cloud manual override (both on and off). `Update_Actuators()` skips threshold logic when flag is `3`, preserving the cloud-commanded state until the next cloud command. When the cloud sends `false`, mode stays `3` (not reverted to `1`) so the auto loop cannot immediately re-engage the actuator.
