# Module Directory Design Spec

**Date:** 2026-04-09  
**Project:** sensors (STM32F103 IoT sensor system)  
**Scope:** Create `module/` directory with 6 independent, Keil-compilable sub-module demos

---

## Goal

Provide each major sub-system of the sensors project as a self-contained, independently compilable and portable Keil MDK-ARM demo. Each demo:

- Can be compiled (F7) and flashed to STM32F103C8T6 independently
- Is **fully self-contained** — all dependencies (fwlib, core, drivers) copied in, no relative path references to parent project
- Implements the **minimum viable validation** of that module's functionality
- Is written for high readability and future reuse as a copy-paste starting point
- README contains detailed technical explanation of how the module works, not just how to build it

---

## Directory Layout

```
sensors/module/
├── dht11/
├── adc_mq/
├── oled/
├── esp8266/
├── mqtt/
└── onenet/
```

Each sub-directory is **fully self-contained** with this structure:

```
module/<name>/
├── <name>.uvprojx     # Standalone Keil project (STM32F103C8, 64KB flash)
├── core/              # CMSIS + startup (copied from ../../core/)
├── fwlib/             # STM32 Standard Peripheral Library (copied from ../../fwlib/)
├── drivers/           # Shared low-level drivers: delay.c, usart.c (copied from ../../hardware/src|inc/)
├── src/               # Module-specific source files
├── inc/               # Module-specific header files
├── demo_main.c        # main() entry point — minimal validation logic
└── README.md          # Technical implementation details + demo usage guide
```

### Why copy instead of relative paths

Copying all dependencies into each module makes the module portable — it can be zipped, moved to another machine, or used in a different project without any path reconfiguration. The tradeoff (duplication of fwlib/core) is acceptable because these files are read-only library code that never changes.

---

## Keil Project Configuration (all modules)

| Setting | Value |
|---|---|
| Target MCU | STM32F103C8 |
| Flash | 64KB (0x08000000–0x08010000) |
| RAM | 20KB (0x20000000–0x20005000) |
| System clock | 72MHz |
| Compiler | ARM Compiler 5.06, -O1 |
| Startup file | `core/startup/arm/startup_stm32f10x_md.s` |
| Debug UART | UART1, PA9 TX / PA10 RX, 115200 baud |
| Heap size | 0x400 (required by MqttKit malloc — set in Options→Target) |
| `stm32f10x_it.c` | **Exclude from all module projects** — each module's `src/` owns its IRQ handlers; no project-level it.c needed |
| Include paths | `core/`, `fwlib/inc/`, `drivers/`, `inc/` |

> **USART2_IRQHandler:** The startup `.s` file declares it `[WEAK]`. `esp8266.c`'s strong definition in `src/` overrides it correctly. No conflict.

---

## Universal `demo_main.c` Init Sequence

Every `demo_main.c` begins with:

```c
SystemInit();        // Configure PLL, set 72 MHz system clock
Delay_Init();        // Must be called before any DelayUs/DelayXms
USART1_Init(115200); // Debug output on PA9/PA10
```

`Delay_Init()` sets the SysTick multipliers. Calling `DelayUs` or `DelayXms` before this produces zero delays.

---

## Module Specifications

### 1. `module/dht11/`

**Purpose:** Validate DHT11 single-wire protocol and temperature/humidity readout.

**Files copied into `src/` and `inc/`:**
- `hardware/src/dht11.c` → `src/dht11.c`
- `hardware/inc/dht11.h` → `inc/dht11.h`

**Demo behavior (`demo_main.c`):**
1. `SystemInit()` → `Delay_Init()` → `USART1_Init(115200)`
2. `DHT11_Init()` — configures PA0 GPIO
3. Loop every 1 second:
   - `ret = DHT11_Read_Data(&temp, &humi)`
   - Success: `UsartPrintf(USART1, "Temp: %d C, Humi: %d %%\r\n", temp, humi)`
   - Error: `UsartPrintf(USART1, "DHT11 read failed (code %d)\r\n", ret)`

**Verification:** UART1 prints temperature and humidity every second.

**Pin mapping:**

| Signal | Pin | Notes |
|---|---|---|
| DHT11 DATA | **PA0** | Header comment says PA11 but macros target PA0 — wire to PA0 |
| UART1 TX | PA9 | 115200 baud |

---

### 2. `module/adc_mq/`

**Purpose:** Validate 5-channel DMA ADC acquisition and MQ sensor percentage calculation.

**Files copied:**
- `hardware/src/adc.c` → `src/adc.c`
- `hardware/inc/adc.h` → `inc/adc.h`
- `hardware/src/mq.c` → `src/mq.c`
- `hardware/inc/mq.h` → `inc/mq.h`

**Demo behavior (`demo_main.c`):**
1. `SystemInit()` → `Delay_Init()` → `USART1_Init(115200)`
2. `ADCx_Init()` — starts DMA continuous circular conversion
3. `DelayXms(100)` — let DMA fill first batch
4. Loop every 1 second: print all 5 channels with raw value and percentage

**Verification:** UART1 prints all ADC readings every second.

**Pin mapping:**

| Signal | Pin | Macro |
|---|---|---|
| MQ3 | PA0 | `ADC_IDX_MQ3` |
| MQ4 | PA1 | `ADC_IDX_MQ4` |
| Light | PA4 | `ADC_IDX_LIGHT` |
| MQ2 | PA5 | `ADC_IDX_MQ2` |
| MQ7 | PA6 | `ADC_IDX_MQ7` |

---

### 3. `module/oled/`

**Purpose:** Validate I2C OLED initialization and character/string rendering.

**Files copied:**
- `OLED/oled.c` → `src/oled.c`
- `OLED/oled.h` → `inc/oled.h`
- `OLED/oledfont.h` → `inc/oledfont.h`

**Demo behavior (`demo_main.c`):**
1. `SystemInit()` → `Delay_Init()` → `USART1_Init(115200)`
2. `OLED_Init()` → `OLED_Clear()`
3. Display `"STM32 OLED Demo"` on line 0, `"Hello World"` on line 2
4. Loop every 500ms: increment counter, display `"Count: XXXX"` on line 4; mirror to UART1

**Verification:** OLED shows text + incrementing counter; UART1 mirrors the count.

**Pin mapping:**

| Signal | Pin |
|---|---|
| OLED SCL | PB1 |
| OLED SDA | PB0 |

---

### 4. `module/esp8266/`

**Purpose:** Validate ESP8266 AT command driver and WiFi association.

**Files copied:**
- `NET/device/src/esp8266.c` → `src/esp8266.c` (**OLED calls replaced with UsartPrintf**)
- `NET/device/inc/esp8266.h` → `inc/esp8266.h`

> **OLED dependency removed:** Original `ESP8266_Init()` calls `OLED_Clear()` / `OLED_ShowString()`. The copied file replaces these with `UsartPrintf(USART1, "[ESP8266] %s\r\n", step)` to remove the OLED hardware dependency.

**Demo behavior (`demo_main.c`):**
1. `SystemInit()` → `Delay_Init()` → `USART1_Init(115200)` → `USART2_Init(115200)`
2. `ESP8266_Init()` — AT → CWMODE=1 → CWDHCP → CWJAP
3. Send `AT+CIFSR\r\n`, print IP address
4. Halt with `"[ESP8266] Demo complete\r\n"`

**Credentials:** `ESP8266_WIFI_INFO` in `src/esp8266.c`

**Pin mapping:**

| Signal | Pin |
|---|---|
| ESP8266 TX → STM32 RX | PA3 (USART2) |
| ESP8266 RX → STM32 TX | PA2 (USART2) |
| UART1 TX | PA9 |

---

### 5. `module/mqtt/`

**Purpose:** Validate MQTT packet encoding offline — no network hardware required.

**Files copied:**
- `NET/MQTT/MqttKit.c` → `src/MqttKit.c`
- `NET/MQTT/MqttKit.h` → `inc/MqttKit.h`
- `NET/MQTT/Common.h` → `inc/Common.h`

> **Heap:** `MQTT_MallocBuffer` maps to `malloc`. Set Keil heap size to **0x400** minimum. Without this, `MQTT_NewBuffer` returns NULL silently.

**Demo behavior (`demo_main.c`):**
1. `SystemInit()` → `Delay_Init()` → `USART1_Init(115200)`
2. Encode CONNECT packet → print hex dump
3. Encode PUBLISH packet (topic=`"test/topic"`, payload=`"hello"`, QoS 0) → print hex dump
4. Decode hardcoded CONNACK bytes `{0x20,0x02,0x00,0x00}` → print result code
5. Free all packets, halt

**Verification:** UART1 prints hex dumps. No WiFi required.

---

### 6. `module/onenet/`

**Purpose:** Validate full OneNET upload pipeline: WiFi → TCP → MQTT auth → publish one datapoint.

**Files copied:**

| Destination | Source |
|---|---|
| `src/onenet.c` | `NET/onenet/src/onenet.c` — **actuator stubs applied** |
| `inc/onenet.h` | `NET/onenet/inc/onenet.h` |
| `src/hmac_sha1.c` | `NET/onenet/src/hmac_sha1.c` |
| `inc/hmac_sha1.h` | `NET/onenet/inc/hmac_sha1.h` |
| `src/base64.c` | `NET/onenet/src/base64.c` |
| `inc/base64.h` | `NET/onenet/inc/base64.h` |
| `src/MqttKit.c` | `NET/MQTT/MqttKit.c` |
| `inc/MqttKit.h` | `NET/MQTT/MqttKit.h` |
| `inc/Common.h` | `NET/MQTT/Common.h` |
| `src/cJSON.c` | `NET/CJSON/cJSON.c` |
| `inc/cJSON.h` | `NET/CJSON/cJSON.h` |
| `src/esp8266.c` | `NET/device/src/esp8266.c` — OLED calls replaced |
| `inc/esp8266.h` | `NET/device/inc/esp8266.h` |
| `inc/stub_actuators.h` | **New file** — stubs for beep/fan/led/mq symbols |

> **Actuator stubs (`stub_actuators.h`):** `onenet.c` references `Beep_Status`, `Fan_Set`, `Led_Set`, `Mq2_GetPercentage()`, etc. Add `inc/stub_actuators.h`:
> ```c
> #define Beep_Status          0
> #define Fan_Set(x)           do {} while(0)
> #define Led_Set(x)           do {} while(0)
> static inline uint8_t Mq2_GetPercentage(void) { return 0; }
> /* add others as needed */
> ```
> Replace `#include "beep.h"`, `#include "fan.h"`, `#include "mq.h"` in copied `onenet.c` with `#include "stub_actuators.h"`.

**Demo behavior (`demo_main.c`):**
1. `SystemInit()` → `Delay_Init()` → `USART1_Init(115200)` → `USART2_Init(115200)`
2. `ESP8266_Init()` → print result
3. TCP connect to `mqtts.heclouds.com:1883` → print result
4. `OneNet_DevLink()` → print CONNACK code
5. `OneNET_Subscribe()` → print result
6. Set `temp=25; humi=60;` (demo globals)
7. `OneNet_SendData()` → print result
8. Poll `OneNet_RevPro()` once for cloud response
9. Halt with `"[OneNET] Demo complete\r\n"`

**Credentials:** `PROID`, `ACCESS_KEY`, `DEVICE_NAME` in `src/onenet.c`; WiFi in `src/esp8266.c`

---

## README.md Content Requirements

Each module README is written in **Chinese** and contains two major sections:

### Section A: 模块实现原理（Technical deep-dive）

This section explains *how the module works*, not just how to build it. It must cover:

1. **整体架构** — role of this module in the full system, one-paragraph overview
2. **核心实现流程** — step-by-step walkthrough of the key algorithm or protocol with code snippets:
   - **dht11:** single-wire timing diagram, reset pulse, bit reading (40µs threshold), 40-bit frame format, checksum validation
   - **adc_mq:** DMA circular mode setup, `RCC_ADCCLKConfig` ordering requirement, ADC rank→macro mapping, raw-to-percentage conversion
   - **oled:** I2C bit-bang protocol (PB0/PB1), command vs data byte distinction, font rendering pipeline
   - **esp8266:** AT command state machine (AT → CWMODE → CWDHCP → CWJAP → CIPSTART), ISR buffer with volatile safety, `memcpy` before `strstr`
   - **mqtt:** CONNECT packet structure (fixed header + variable header + payload), PUBLISH packet encoding, HMAC-SHA1 auth token format
   - **onenet:** full upload pipeline (sensor → JSON → MQTT PUBLISH → ESP8266 AT+CIPSEND → TCP → cloud), cloud command receive/parse flow, actuator override mode=3 logic
3. **关键设计要点** — pitfalls and non-obvious constraints (e.g., ADC clock ordering, volatile isolation, HMAC key decoding)
4. **数据格式** — for onenet: exact JSON payload format, MQTT topic paths, auth token structure

### Section B: Demo 使用指南（Usage guide）

1. **引脚配置表** — hardware wiring
2. **配置项** — what to change before building (credentials, SSID, thresholds)
3. **编译与烧录** — open `.uvprojx` → F7 → ST-Link flash
4. **预期输出** — exactly what to see on UART1 / OLED when demo runs correctly
5. **源码修改说明** — documents any changes from original (OLED stripped, stubs added)
6. **复用说明** — which files to copy and which functions to call to integrate this module

---

## Code Style Rules (all modules)

1. **File header** at top of every `demo_main.c`:
   ```c
   /**
    * @file    demo_main.c
    * @brief   <Module> demo — <one-line description>
    * @board   STM32F103C8T6 @ 72 MHz
    * @pins    <key pin assignments>
    * @uart    UART1 PA9/PA10 115200 baud (debug output)
    * @note    <credentials or config to change before use>
    */
   ```
2. **Three-phase structure:** Init → Run → Loop/Halt
3. Every non-trivial step preceded by `UsartPrintf` status line
4. Named `#define` for all magic numbers
5. `#ifndef` include guards in all headers
6. Source file changes from originals documented in a `/* MODULE CHANGE: ... */` comment at the change site
