# AGENTS.md

This document gives working guidance for coding agents operating in `zk/misc/sensors`.

## Scope

This project is an STM32F103C8T6 environmental monitoring firmware built with Keil MDK-ARM V5 and STM32 Standard Peripheral Library. The firmware:

- reads DHT11, light, and MQ gas sensors
- shows values on an OLED
- connects to WiFi through ESP8266 on USART2
- uploads telemetry to OneNET over MQTT
- accepts cloud property commands
- drives fan, buzzer, and LED outputs from threshold logic or cloud override

This file is the preferred local guide for agents working on this project.

## Build And Toolchain

- IDE: Keil MDK-ARM V5
- Compiler: ARM Compiler 5.06
- Project file: `stm32f103.uvprojx`
- Main target: `STM32F103`
- Build output directory: `output/`
- Listing output directory: `listing/`

Do not assume GCC, CMake, Makefile, or modern C99/C11 support.

### Compiler constraints

- Prefer C89/C90-compatible code.
- Avoid `inline`, mixed declarations and statements, designated initializers, `//`-heavy rewrites in old files, and other newer-language features unless already proven to compile under ARMCC 5.
- Keep helper functions `static` instead of `static inline` unless the project already uses a compiler-safe compatibility macro.

## Tracking Rules

Tracked and shared:

- `*.uvprojx`
- source under `user/`, `hardware/`, `NET/`, `OLED/`, `core/`
- project documentation

Ignored and should stay untracked:

- `output/`
- `listing/`
- `*.uvguix.*`
- `*.uvoptx`

Do not re-add build artifacts or user-specific Keil state files.

## High-Level Layout

```text
user/main.c              Main application loop and top-level logic
hardware/inc|src/        Sensor and actuator drivers
NET/device/              ESP8266 AT-command transport
NET/MQTT/                MQTT packet encode/decode
NET/onenet/              OneNET auth, publish, subscribe, command handling
NET/CJSON/               JSON parser used for cloud commands
OLED/                    OLED driver and fonts
core/                    CMSIS/system/startup/interrupt code
fwlib/                   STM32 Standard Peripheral Library
RTE/ DebugConfig/        Keil-generated support files
```

## Main Runtime Flow

Entry point is `user/main.c`.

Startup sequence:

1. `Hardware_Init()`
2. `ESP8266_Init()`
3. Open TCP connection to OneNET broker
4. `OneNet_DevLink()`
5. `OneNET_Subscribe()`
6. Enter polling loop with `DelayMs(25)`

Steady-state loop behavior:

- Every 25 ms:
  - poll ESP8266 receive buffer
  - process cloud commands via `OneNet_RevPro()`
  - run `Update_Actuators()`
- Every `UPLOAD_TICKS` iterations:
  - read sensors via `Refresh_Sensors()`
  - upload data with `OneNet_SendData()` if DHT11 succeeded
  - refresh OLED
  - clear ESP8266 buffer

`UPLOAD_TICKS` is 200, so upload cadence is about 5 seconds.

## Sensor And GPIO Mapping

### ADC/DMA channels actually used

Defined in `hardware/inc/adc.h` and configured in `hardware/src/adc.c`.

- `PA0` -> `ADC_IDX_MQ3`
- `PA1` -> `ADC_IDX_MQ4`
- `PA4` -> `ADC_IDX_LIGHT`
- `PA5` -> `ADC_IDX_MQ2`
- `PA6` -> `ADC_IDX_MQ7`

`NOFCHANEL` is 5. DMA buffer is `ADC_ConvertedValue[NOFCHANEL]`.

Always use `ADC_IDX_*` macros. Do not use raw numeric indices.

### Pins reserved and unavailable

- `PA2` / `PA3` are USART2 TX/RX for ESP8266
- therefore `MQ5` and `MQ6` are intentionally unavailable in this hardware configuration
- `hardware/src/mq.c` exposes `Mq5_*()` and `Mq6_*()` as stubs returning zero

### Other important pins

- DHT11: `PA11`
- Fan: `PA8`
- Buzzer: `PC13`
- LED / grow-light output: `PC14`
- Irrigation output may exist on `PC15` in historical docs, but verify before using
- OLED: `PB0`/`PB1`
- Debug UART: USART1 on `PA9`/`PA10`
- ESP8266: USART2 on `PA2`/`PA3`

## Data Model And Control Logic

Global readings in `user/main.c`:

- `temp`
- `humi`
- `light_pct`
- `mq2_vol`

Thresholds:

- `temp_adj`
- `humi_adj`
- `smog_th`
- `light_th`

Actuator mode flags:

- `fan_mode`
- `led_mode`

Mode values:

- `1`: automatic threshold-driven control
- `3`: cloud manual override, preserved until a later cloud command changes it

Behavior:

- buzzer trips on temperature, humidity, or smoke threshold
- fan trips on temperature or humidity threshold
- LED trips when light falls below threshold
- when a mode is `3`, auto logic must not overwrite the cloud-selected output state

## Display Behavior

`user/main.c` currently defines `DISPLAY_MODE_GAS`.

That means OLED shows:

- `MQ2`
- `MQ3`
- `MQ4`
- `MQ7`

If that macro is removed, the fallback display is:

- temperature
- humidity
- light

Do not assume both views are active simultaneously.

## Network Stack

### ESP8266 transport

Implemented in `NET/device/src/esp8266.c`.

Important facts:

- WiFi credentials are hardcoded in `ESP8266_WIFI_INFO`
- receive buffer is `volatile unsigned char esp8266_buf[512]`
- receive count variables are also `volatile`
- `USART2_IRQHandler` appends incoming bytes into that buffer

### OneNET layer

Implemented in `NET/onenet/src/onenet.c`.

Important facts:

- product credentials are hardcoded with `PROID`, `ACCESS_KEY`, `DEVICE_NAME`
- telemetry fields currently sent are `temp`, `humi`, `light`, `beep`
- subscribed topic is `$sys/{PROID}/{DEVICE_NAME}/thing/property/set`
- handled cloud properties currently include:
  - `fan`
  - `led`
  - `light_th`
  - `temp_adj`
  - `humi_adj`

Note that `smog_th` is local only unless you explicitly add it to cloud schema and parser.

## Critical Modification Rules

### ADC and DMA

- `RCC_ADCCLKConfig(...)` must be called before `ADC_Init(...)`
- ADC clock must stay within STM32F1 spec
- if you add or reorder ADC channels, update all three places together:
  - `NOFCHANEL`
  - `ADC_IDX_*` macros
  - `ADC_RegularChannelConfig(...)` rank order

Any mismatch here corrupts sensor mapping silently.

### Volatile and ISR safety

`esp8266_buf`, `esp8266_cnt`, and `esp8266_cntPre` are ISR-updated state.

Rules:

- do not pass the volatile buffer directly into libc routines that expect stable memory unless you first snapshot it
- preserve the existing pattern used in `ESP8266_SendCmd()`
- be careful when clearing or scanning the buffer while USART2 interrupts may still be active

### Cloud command parsing

- `OneNet_RevPro()` uses `cJSON`
- clean up allocated cJSON objects on all exit paths
- do not fall through MQTT packet type cases accidentally

### Fault handlers

`core/stm32f10x_it.c` prints fault names over debug UART and then loops forever. Keep this behavior unless there is a strong reason to change it.

## Files To Treat Carefully

- `fwlib/`: vendor library, avoid edits unless absolutely necessary
- `core/startup/arm/*.s`: startup files, change only for target/startup work
- `stm32f103.uvprojx`: shared project definition, track meaningful build/source changes
- `NET/onenet/src/onenet.c`: contains secrets; avoid leaking credentials in logs or commits

## Known Project Quirks

- Some comments and docs are mojibake because of legacy encoding; do not “clean up” encoding blindly across the tree.
- `hardware/inc/sys.c` exists with a suspicious path/name combination. Treat it as a legacy anomaly and inspect before changing or deleting it.
- `README.md` and `CLAUDE.md` are useful references, but code is the source of truth if docs drift.
- ARMCC 5 compatibility matters more than stylistic modernization.

## Recommended Workflow For Agents

1. Read `user/main.c` first for current behavior.
2. If touching sensors, inspect `hardware/inc/adc.h`, `hardware/src/adc.c`, and `hardware/src/mq.c` together.
3. If touching cloud or WiFi logic, inspect `NET/device/src/esp8266.c` and `NET/onenet/src/onenet.c` together.
4. If touching shared project config, verify whether the change belongs in `stm32f103.uvprojx` or only in ignored Keil user files.
5. Prefer minimal, localized edits because this codebase is tightly coupled and lightly abstracted.

## Validation Expectations

Preferred validation:

- rebuild target `STM32F103` in Keil
- watch for ARMCC compatibility errors first
- if sensor mapping changed, verify OLED/debug UART values against expected channel order
- if network logic changed, verify:
  - ESP8266 can join WiFi
  - MQTT connect still succeeds
  - property set commands still parse

If local rebuild is not available, state that clearly.

## Commit Style

When creating commits for this project, prefix the message with the folder name:

- `[sensors] fix ARMCC build in mq helper`
- `[sensors] stop tracking build artifacts`

Keep commits focused. Do not mix source changes with generated-file cleanup.
