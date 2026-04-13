# Module Demos Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create `module/` directory containing 6 fully self-contained, independently Keil-compilable demo sub-projects for the sensors repo (dht11, adc_mq, oled, esp8266, mqtt, onenet).

**Architecture:** Each module lives under `sensors/module/<name>/` and bundles its own copy of all dependencies (fwlib, core, drivers) — no relative-path references to the parent project. Each has a `demo_main.c` with a minimal validation demo, and a detailed Chinese README covering implementation internals + usage guide.

**Tech Stack:** STM32F103C8T6, Keil MDK-ARM V5, ARM Compiler 5.06, STM32 Standard Peripheral Library, ESP8266 AT commands, MQTT (MqttKit), OneNET IoT platform.

**Spec:** `docs/superpowers/specs/2026-04-09-module-design.md`

---

## File Structure

```
module/
├── dht11/
│   ├── dht11.uvprojx
│   ├── core/                   # copied from ../../core/
│   ├── fwlib/                  # copied from ../../fwlib/
│   ├── drivers/                # delay.c/.h, usart.c/.h
│   ├── src/dht11.c
│   ├── inc/dht11.h
│   ├── demo_main.c
│   └── README.md
├── adc_mq/
│   ├── adc_mq.uvprojx
│   ├── core/, fwlib/, drivers/
│   ├── src/adc.c, mq.c
│   ├── inc/adc.h, mq.h
│   ├── demo_main.c
│   └── README.md
├── oled/
│   ├── oled.uvprojx
│   ├── core/, fwlib/, drivers/
│   ├── src/oled.c
│   ├── inc/oled.h, oledfont.h
│   ├── demo_main.c
│   └── README.md
├── esp8266/
│   ├── esp8266.uvprojx
│   ├── core/, fwlib/, drivers/
│   ├── src/esp8266.c           # OLED calls replaced with UsartPrintf
│   ├── inc/esp8266.h
│   ├── demo_main.c
│   └── README.md
├── mqtt/
│   ├── mqtt.uvprojx
│   ├── core/, fwlib/, drivers/
│   ├── src/MqttKit.c
│   ├── inc/MqttKit.h, Common.h
│   ├── demo_main.c
│   └── README.md
└── onenet/
    ├── onenet.uvprojx
    ├── core/, fwlib/, drivers/
    ├── src/                    # onenet.c(stubbed), esp8266.c, MqttKit.c,
    │                           # hmac_sha1.c, base64.c, cJSON.c
    ├── inc/                    # all headers + stub_actuators.h
    ├── demo_main.c
    └── README.md
```

---

## Shared Context for All Tasks

**Universal init sequence** every `demo_main.c` uses:
```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
Delay_Init();
Usart1_Init(115200);
```

> **Note on `SystemInit()`:** The spec lists `SystemInit()` as the first call, but the startup file `startup_stm32f10x_md.s` already calls `SystemInit()` before `main()` — it must NOT be called again in `demo_main.c`. The init sequence above is correct.

**UART debug macro** (`usart.h` already defines `USART_DEBUG` as `USART1`):
```c
UsartPrintf(USART_DEBUG, "[tag] message\r\n");
```

**Keil project settings** (all modules):
- MCU: STM32F103C8, Flash 64KB, RAM 20KB
- Compiler: ARM Compiler 5.06, -O1
- Startup: `core/startup/arm/startup_stm32f10x_md.s`
- Heap: 0x400 (needed by MqttKit malloc)
- Include paths: `core/`, `fwlib/inc/`, `drivers/`, `inc/`
- **Exclude** `stm32f10x_it.c` — IRQ handlers live in module src files

**Copying shared deps:** Each task begins by copying `fwlib/`, `core/`, `delay.c/.h`, `usart.c/.h` into the module directory. The Keil `.uvprojx` references these local copies — no path outside the module directory.

**File header template** for all `demo_main.c`:
```c
/**
 * @file    demo_main.c
 * @brief   <Module> demo — <one-line description>
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    <key pins>
 * @uart    UART1 PA9/PA10 115200 baud (debug output)
 * @note    <config to change before use, if any>
 */
```

---

## Task 1: module/dht11 — DHT11 温湿度传感器 Demo

**Files:**
- Create: `module/dht11/demo_main.c`
- Create: `module/dht11/src/dht11.c` (copied from `hardware/src/dht11.c`)
- Create: `module/dht11/inc/dht11.h` (copied from `hardware/inc/dht11.h`)
- Create: `module/dht11/drivers/` (delay.c/.h, usart.c/.h from `hardware/src|inc/`)
- Create: `module/dht11/core/` (copied from `core/`)
- Create: `module/dht11/fwlib/` (copied from `fwlib/`)
- Create: `module/dht11/dht11.uvprojx`
- Create: `module/dht11/README.md`

- [ ] **Step 1: Copy shared dependencies**

```bash
# Run from sensors/ root
cp -r core   module/dht11/core
cp -r fwlib  module/dht11/fwlib
mkdir -p module/dht11/drivers
cp hardware/src/delay.c  module/dht11/drivers/
cp hardware/inc/delay.h  module/dht11/drivers/
cp hardware/src/usart.c  module/dht11/drivers/
cp hardware/inc/usart.h  module/dht11/drivers/
```

- [ ] **Step 2: Copy module source files**

```bash
mkdir -p module/dht11/src module/dht11/inc
cp hardware/src/dht11.c  module/dht11/src/
cp hardware/inc/dht11.h  module/dht11/inc/
```

- [ ] **Step 3: Write `module/dht11/demo_main.c`**

```c
/**
 * @file    demo_main.c
 * @brief   DHT11 demo — 循环读取温湿度并通过 UART1 输出
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    DHT11 DATA → PA0 (注意: 头文件注释写 PA11, 宏实际操作 PA0)
 *          UART1 TX   → PA9  (115200 baud, 调试输出)
 * @note    无需额外配置，上电即运行
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "dht11.h"

#define READ_INTERVAL_MS  1000   /* 读取间隔, ms */

int main(void)
{
    uint8_t temp = 0, humi = 0;
    uint8_t ret  = 0;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[DHT11] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[DHT11] Pin: PA0, Interval: %d ms\r\n", READ_INTERVAL_MS);

    DHT11_Init();
    UsartPrintf(USART_DEBUG, "[DHT11] Init OK\r\n");

    /* ---- Loop ---- */
    while(1)
    {
        ret = DHT11_Read_Data(&temp, &humi);

        if(ret == 0)
        {
            UsartPrintf(USART_DEBUG, "[DHT11] Temp: %d C, Humi: %d %%\r\n", temp, humi);
        }
        else
        {
            UsartPrintf(USART_DEBUG, "[DHT11] Read failed (code %d)\r\n", ret);
        }

        DelayXms(READ_INTERVAL_MS);
    }
}
```

- [ ] **Step 4: Create Keil project `module/dht11/dht11.uvprojx`**

Create a Keil MDK-ARM project file with the following source groups and files:

**Source Groups:**
- `Startup`: `core/startup/arm/startup_stm32f10x_md.s`
- `Core`: `core/system_stm32f10x.c`
- `Fwlib`: all `.c` files in `fwlib/src/` (standard peripheral library)
- `Drivers`: `drivers/delay.c`, `drivers/usart.c`
- `Module`: `src/dht11.c`
- `App`: `demo_main.c`

**Include Paths:** `core/;fwlib/inc/;drivers/;inc/`

**Preprocessor Defines:** `STM32F10X_MD,USE_STDPERIPH_DRIVER`

**Target:** STM32F103C8, 64KB Flash (0x08000000), 20KB RAM (0x20000000), Heap 0x400

- [ ] **Step 5: Write `module/dht11/README.md`**

README 分两部分（见下方模板），包含：
- 模块实现原理（DHT11 单总线时序、40-bit 帧格式、校验逻辑、PA0 与注释不符说明）
- Demo 使用指南（引脚配置、编译烧录、预期输出）

内容参考：

```markdown
# DHT11 温湿度传感器模块

## 功能说明

DHT11 是单总线数字温湿度传感器，通过一根数据线完成供电确认和数据传输。
本模块封装了完整的单总线驱动，每次调用 `DHT11_Read_Data()` 读取一帧 40-bit 数据，
返回整数温度（°C）和湿度（%RH）。

---

## 模块实现原理

### 整体架构

DHT11 通信全部在一根 GPIO 引脚上完成（本项目 PA0）：
- STM32 拉低 18ms → 拉高 20-40µs → 切换为输入，等待传感器应答
- DHT11 应答：拉低 40-80µs → 拉高 40-80µs
- 随后发送 40-bit 数据（MSB 优先，5 字节）

### 核心实现流程

**复位与应答检测（DHT11_Rst + DHT11_Check）**
```c
void DHT11_Rst(void) {
    DHT11_IO_OUT();
    DHT11_DQ_OUT(0);  // 拉低至少 18ms
    DelayXms(20);
    DHT11_DQ_OUT(1);  // 拉高 20-40µs
    DelayUs(30);
}

uint8_t DHT11_Check(void) {
    DHT11_IO_IN();
    // 等待 DHT11 拉低 (40-80µs)
    while (DHT11_DQ_IN && retry < 100) { retry++; DelayUs(1); }
    // 等待 DHT11 拉高 (40-80µs)
    while (!DHT11_DQ_IN && retry < 100) { retry++; DelayUs(1); }
    return 0; // 检测成功
}
```

**位读取（DHT11_Read_Bit）**

DHT11 用高电平持续时间区分 0/1：
- 低电平起始 → 拉高 → 等待 40µs → 采样
  - 高电平 < 40µs → bit = 0（低电平约 26-28µs）
  - 高电平 > 40µs → bit = 1（高电平约 70µs）

```c
uint8_t DHT11_Read_Bit(void) {
    while (DHT11_DQ_IN && retry < 100) { ... }   // 等待下降沿
    while (!DHT11_DQ_IN && retry < 100) { ... }  // 等待上升沿
    DelayUs(40);
    return DHT11_DQ_IN ? 1 : 0;  // 40µs 后采样
}
```

**40-bit 帧格式**

| 字节 | 内容 |
|------|------|
| buf[0] | 湿度整数 |
| buf[1] | 湿度小数（DHT11 恒为 0） |
| buf[2] | 温度整数 |
| buf[3] | 温度小数（DHT11 恒为 0） |
| buf[4] | 校验 = buf[0]+buf[1]+buf[2]+buf[3] |

**校验逻辑**
```c
if ((buf[0]+buf[1]+buf[2]+buf[3]) == buf[4]) {
    *humi = buf[0];
    *temp = buf[2];
    return 0; // 成功
}
return 2; // 校验失败
```

### 关键设计要点

1. **PA0 与注释不符**：`dht11.h` 注释写 `PA11`，但宏 `DHT11_DQ_OUT`/`DHT11_DQ_IN` 操作 `GPIO_Pin_0`（PA0），CRL 也操作 bits[3:0]。**实际接线接 PA0**。
2. **开漏输出 + 上拉**：GPIO 配置为 `GPIO_Mode_Out_OD`，需要外部 4.7kΩ 上拉电阻至 3.3V。
3. **时序依赖 Delay_Init()**：所有 `DelayUs`/`DelayXms` 调用依赖 SysTick 系数，**必须先调用 `Delay_Init()`**。

---

## 引脚配置

| 信号 | STM32 引脚 | 说明 |
|------|-----------|------|
| DHT11 DATA | **PA0** | 需 4.7kΩ 上拉至 3.3V |
| UART1 TX (调试) | PA9 | 115200 baud，接串口工具 |
| UART1 RX | PA10 | 可不接 |

## Demo 行为

上电后每 1 秒在 UART1 输出一行：

```
[DHT11] Demo start
[DHT11] Pin: PA0, Interval: 1000 ms
[DHT11] Init OK
[DHT11] Temp: 25 C, Humi: 60 %
[DHT11] Temp: 25 C, Humi: 61 %
...
```

读取失败时输出：
```
[DHT11] Read failed (code 1)   ← 传感器未响应
[DHT11] Read failed (code 2)   ← 校验错误
```

## 编译与烧录

1. 用 Keil MDK-ARM V5 打开 `dht11.uvprojx`
2. 按 F7 编译（无警告无错误）
3. 通过 ST-Link 连接，按下载按钮烧录
4. 打开串口工具（115200,8N1）观察输出

## 配置项

无需修改，默认读取间隔 1000ms（`demo_main.c` 中 `READ_INTERVAL_MS` 宏可调整）。

## 复用说明

将以下文件复制到目标工程并添加到 Keil 工程：
- `src/dht11.c`
- `inc/dht11.h`

调用方式：
```c
DHT11_Init();
uint8_t t, h;
if (DHT11_Read_Data(&t, &h) == 0) {
    // t = 温度, h = 湿度
}
```
```

- [ ] **Step 6: Commit**

```bash
git add module/dht11/
git commit -m "[sensors] add module/dht11 demo"
```

---

## Task 2: module/adc_mq — ADC + MQ 气体传感器 Demo

**Files:**
- Create: `module/adc_mq/demo_main.c`
- Create: `module/adc_mq/src/adc.c`, `src/mq.c`
- Create: `module/adc_mq/inc/adc.h`, `inc/mq.h`
- Create: `module/adc_mq/drivers/`, `core/`, `fwlib/`
- Create: `module/adc_mq/adc_mq.uvprojx`
- Create: `module/adc_mq/README.md`

- [ ] **Step 1: Copy shared dependencies and module sources**

```bash
cp -r core  module/adc_mq/core
cp -r fwlib module/adc_mq/fwlib
mkdir -p module/adc_mq/drivers
cp hardware/src/delay.c  module/adc_mq/drivers/
cp hardware/inc/delay.h  module/adc_mq/drivers/
cp hardware/src/usart.c  module/adc_mq/drivers/
cp hardware/inc/usart.h  module/adc_mq/drivers/
mkdir -p module/adc_mq/src module/adc_mq/inc
cp hardware/src/adc.c  module/adc_mq/src/
cp hardware/inc/adc.h  module/adc_mq/inc/
cp hardware/src/mq.c   module/adc_mq/src/
cp hardware/inc/mq.h   module/adc_mq/inc/
```

- [ ] **Step 2: Write `module/adc_mq/demo_main.c`**

```c
/**
 * @file    demo_main.c
 * @brief   ADC + MQ 传感器 demo — DMA 5 通道采集，循环打印原始值和百分比
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    MQ3→PA0, MQ4→PA1, LIGHT→PA4, MQ2→PA5, MQ7→PA6 (ADC 模拟输入)
 *          UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    PA2/PA3 已被 USART2 占用，MQ5/MQ6 无法接入
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "adc.h"
#include "mq.h"

#define PRINT_INTERVAL_MS  1000   /* 打印间隔, ms */

int main(void)
{
    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[ADC_MQ] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[ADC_MQ] 5-channel DMA ADC: MQ3(PA0) MQ4(PA1) LIGHT(PA4) MQ2(PA5) MQ7(PA6)\r\n");

    ADCx_Init();
    DelayXms(100);   /* 等待 DMA 填充首批数据 */
    UsartPrintf(USART_DEBUG, "[ADC_MQ] ADC+DMA init OK\r\n\r\n");

    /* ---- Loop ---- */
    while(1)
    {
        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ3 (PA0) Alcohol  : raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ3],  Mq3_GetPercentage());

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ4 (PA1) Methane  : raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ4],  Mq4_GetPercentage());

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] LIGHT(PA4) Luminance: raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_LIGHT], (int)(ADC_ConvertedValue[ADC_IDX_LIGHT] / 4095.0f * 100));

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ2 (PA5) Smoke    : raw=%4d  %2d%%\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ2],  Mq2_GetPercentage());

        UsartPrintf(USART_DEBUG,
            "[ADC_MQ] MQ7 (PA6) CO       : raw=%4d  %2d%%\r\n\r\n",
            ADC_ConvertedValue[ADC_IDX_MQ7],  Mq7_GetPercentage());

        DelayXms(PRINT_INTERVAL_MS);
    }
}
```

- [ ] **Step 3: Create Keil project `module/adc_mq/adc_mq.uvprojx`**

Same structure as dht11. Source groups:
- `Startup`: `core/startup/arm/startup_stm32f10x_md.s`
- `Core`: `core/system_stm32f10x.c`
- `Fwlib`: all `.c` in `fwlib/src/`
- `Drivers`: `drivers/delay.c`, `drivers/usart.c`
- `Module`: `src/adc.c`, `src/mq.c`
- `App`: `demo_main.c`

Include paths: `core/;fwlib/inc/;drivers/;inc/`
Defines: `STM32F10X_MD,USE_STDPERIPH_DRIVER`

- [ ] **Step 4: Write `module/adc_mq/README.md`**

README 包含：
- **模块实现原理**：ADC1+DMA1 连续循环模式工作流程；`RCC_ADCCLKConfig` 必须在 `ADC_Init` 前调用的原因（ADC 时钟上限 14MHz，默认 36MHz 违规）；DMA 循环传输到 `ADC_ConvertedValue[5]`；Rank→宏的映射关系；原始值转百分比公式（`raw/4095*100`）；PA2/PA3 被 USART2 占用无法用于 ADC 的说明
- **Demo 使用指南**：引脚表、编译烧录步骤、预期串口输出格式

- [ ] **Step 5: Commit**

```bash
git add module/adc_mq/
git commit -m "[sensors] add module/adc_mq demo"
```

---

## Task 3: module/oled — OLED 显示 Demo

**Files:**
- Create: `module/oled/demo_main.c`
- Create: `module/oled/src/oled.c`
- Create: `module/oled/inc/oled.h`, `inc/oledfont.h`
- Create: `module/oled/drivers/`, `core/`, `fwlib/`
- Create: `module/oled/oled.uvprojx`
- Create: `module/oled/README.md`

- [ ] **Step 1: Copy shared dependencies and module sources**

```bash
cp -r core  module/oled/core
cp -r fwlib module/oled/fwlib
mkdir -p module/oled/drivers
cp hardware/src/delay.c  module/oled/drivers/
cp hardware/inc/delay.h  module/oled/drivers/
cp hardware/src/usart.c  module/oled/drivers/
cp hardware/inc/usart.h  module/oled/drivers/
mkdir -p module/oled/src module/oled/inc
cp OLED/oled.c       module/oled/src/
cp OLED/oled.h       module/oled/inc/
cp OLED/oledfont.h   module/oled/inc/
```

- [ ] **Step 2: Write `module/oled/demo_main.c`**

```c
/**
 * @file    demo_main.c
 * @brief   OLED demo — 显示静态标题 + 自增计数器，UART1 同步输出
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    OLED SCL→PB1, OLED SDA→PB0 (软件 I2C，bit-bang)
 *          UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    OLED 使用软件 I2C，无需额外硬件配置
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"

#define REFRESH_INTERVAL_MS  500    /* 刷新间隔, ms */
#define COUNT_MAX            9999   /* 计数上限，循环归零 */

int main(void)
{
    unsigned short count = 0;
    char buf[8];

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[OLED] Demo start\r\n");

    OLED_Init();
    OLED_Clear();

    /* 静态内容 */
    OLED_ShowString(0, 0, "STM32 OLED Demo", 16);
    OLED_ShowString(0, 2, "Hello World", 16);
    UsartPrintf(USART_DEBUG, "[OLED] Static content displayed\r\n");

    /* ---- Loop ---- */
    while(1)
    {
        snprintf(buf, sizeof(buf), "%4d", count);
        OLED_ShowString(0, 4, "Count:", 16);
        OLED_ShowString(64, 4, buf, 16);

        UsartPrintf(USART_DEBUG, "[OLED] Count: %d\r\n", count);

        count = (count >= COUNT_MAX) ? 0 : count + 1;

        DelayXms(REFRESH_INTERVAL_MS);
    }
}
```

- [ ] **Step 3: Create Keil project `module/oled/oled.uvprojx`**

Source groups:
- `Startup`: `core/startup/arm/startup_stm32f10x_md.s`
- `Core`: `core/system_stm32f10x.c`
- `Fwlib`: all `.c` in `fwlib/src/`
- `Drivers`: `drivers/delay.c`, `drivers/usart.c`
- `Module`: `src/oled.c`
- `App`: `demo_main.c`

Include paths: `core/;fwlib/inc/;drivers/;inc/`

- [ ] **Step 4: Write `module/oled/README.md`**

README 包含：
- **模块实现原理**：软件 I2C bit-bang 实现（PB0=SDA, PB1=SCL）；OLED 命令 vs 数据字节区分（Co/D/C# 位）；字符渲染流程（字模查表→逐列写入 GDDRAM）；`OLED_ShowString` 如何组织多字符显示；显示坐标系（列 0-127，行 0-7，字体高度=行数）
- **Demo 使用指南**：接线表、编译烧录、预期显示内容

- [ ] **Step 5: Commit**

```bash
git add module/oled/
git commit -m "[sensors] add module/oled demo"
```

---

## Task 4: module/esp8266 — ESP8266 WiFi 驱动 Demo

**Files:**
- Create: `module/esp8266/demo_main.c`
- Create: `module/esp8266/src/esp8266.c` (OLED calls stripped)
- Create: `module/esp8266/inc/esp8266.h`
- Create: `module/esp8266/drivers/`, `core/`, `fwlib/`
- Create: `module/esp8266/esp8266.uvprojx`
- Create: `module/esp8266/README.md`

- [ ] **Step 1: Copy shared dependencies**

```bash
cp -r core  module/esp8266/core
cp -r fwlib module/esp8266/fwlib
mkdir -p module/esp8266/drivers
cp hardware/src/delay.c  module/esp8266/drivers/
cp hardware/inc/delay.h  module/esp8266/drivers/
cp hardware/src/usart.c  module/esp8266/drivers/
cp hardware/inc/usart.h  module/esp8266/drivers/
mkdir -p module/esp8266/src module/esp8266/inc
cp NET/device/inc/esp8266.h  module/esp8266/inc/
```

- [ ] **Step 2: Copy and patch `esp8266.c` — strip OLED calls**

Copy `NET/device/src/esp8266.c` to `module/esp8266/src/esp8266.c`, then make these changes in `ESP8266_Init()`:

Replace every `OLED_Clear()` / `OLED_ShowString(...)` call with a `UsartPrintf` equivalent. Add a `/* MODULE CHANGE: OLED calls replaced with UsartPrintf for portability */` comment at the top of the function. Also remove `#include "oled.h"` if present.

The patched `ESP8266_Init()` should print step progress like:
```c
/* MODULE CHANGE: OLED calls replaced with UsartPrintf for portability */
void ESP8266_Init(void)
{
    Usart2_Init(115200);

    UsartPrintf(USART_DEBUG, "[ESP8266] Init start\r\n");

    while(ESP8266_SendCmd("AT\r\n", "OK"))
        DelayXms(500);
    UsartPrintf(USART_DEBUG, "[ESP8266] 1/4 AT OK\r\n");

    while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
        DelayXms(500);
    UsartPrintf(USART_DEBUG, "[ESP8266] 2/4 CWMODE=1 OK\r\n");

    while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
        DelayXms(500);
    UsartPrintf(USART_DEBUG, "[ESP8266] 3/4 CWDHCP OK\r\n");

    while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "OK"))
        DelayXms(500);
    UsartPrintf(USART_DEBUG, "[ESP8266] 4/4 WiFi connected OK\r\n");
}
```

- [ ] **Step 3: Write `module/esp8266/demo_main.c`**

```c
/**
 * @file    demo_main.c
 * @brief   ESP8266 demo — WiFi 连接并打印 IP 地址
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    ESP8266 TX→PA3(USART2 RX), ESP8266 RX→PA2(USART2 TX)
 *          UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    修改 src/esp8266.c 中 ESP8266_WIFI_INFO 宏为实际 WiFi SSID 和密码
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "esp8266.h"

#define CIFSR_CMD   "AT+CIFSR\r\n"   /* 查询本机 IP 地址 */

int main(void)
{
    unsigned char *resp = NULL;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[ESP8266] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[ESP8266] USART2: PA2(TX) PA3(RX) 115200 baud\r\n");

    /* ---- Run ---- */
    ESP8266_Init();   /* blocks until WiFi connected */

    /* Query and print IP address */
    UsartPrintf(USART_DEBUG, "[ESP8266] Querying IP (AT+CIFSR)...\r\n");
    ESP8266_SendCmd(CIFSR_CMD, "STAIP");   /* signature: (char *cmd, char *res), no timeout param */
    DelayXms(200);
    resp = ESP8266_GetIPD(0);
    if(resp != NULL)
        UsartPrintf(USART_DEBUG, "[ESP8266] Response: %s\r\n", resp);
    else
        UsartPrintf(USART_DEBUG, "[ESP8266] No response to AT+CIFSR\r\n");

    ESP8266_Clear();

    /* ---- Halt ---- */
    UsartPrintf(USART_DEBUG, "[ESP8266] Demo complete\r\n");
    while(1) { }
}
```

- [ ] **Step 4: Create Keil project `module/esp8266/esp8266.uvprojx`**

Source groups:
- `Startup`, `Core`, `Fwlib` (same as previous modules)
- `Drivers`: `drivers/delay.c`, `drivers/usart.c`
- `Module`: `src/esp8266.c`
- `App`: `demo_main.c`

Include paths: `core/;fwlib/inc/;drivers/;inc/`

- [ ] **Step 5: Write `module/esp8266/README.md`**

README 包含：
- **模块实现原理**：AT 指令状态机（AT→CWMODE→CWDHCP→CWJAP→CIPSTART）；`ESP8266_SendCmd` 内部 `memcpy` 到本地 buffer 再调 `strstr` 的原因（volatile ISR buffer 不能直接传给标准库函数）；USART2_IRQHandler 写入 volatile 缓冲的机制；`ESP8266_SendData` 流程（AT+CIPSEND=N → 等待">"提示符 → 发送二进制数据）；OLED 调用被替换的说明
- **Demo 使用指南**：WiFi 配置项、接线表、预期输出

- [ ] **Step 6: Commit**

```bash
git add module/esp8266/
git commit -m "[sensors] add module/esp8266 demo"
```

---

## Task 5: module/mqtt — MQTT 协议封包 Demo

**Files:**
- Create: `module/mqtt/demo_main.c`
- Create: `module/mqtt/src/MqttKit.c`
- Create: `module/mqtt/inc/MqttKit.h`, `inc/Common.h`
- Create: `module/mqtt/drivers/`, `core/`, `fwlib/`
- Create: `module/mqtt/mqtt.uvprojx`
- Create: `module/mqtt/README.md`

- [ ] **Step 1: Copy shared dependencies and module sources**

```bash
cp -r core  module/mqtt/core
cp -r fwlib module/mqtt/fwlib
mkdir -p module/mqtt/drivers
cp hardware/src/delay.c  module/mqtt/drivers/
cp hardware/inc/delay.h  module/mqtt/drivers/
cp hardware/src/usart.c  module/mqtt/drivers/
cp hardware/inc/usart.h  module/mqtt/drivers/
mkdir -p module/mqtt/src module/mqtt/inc
cp NET/MQTT/MqttKit.c  module/mqtt/src/
cp NET/MQTT/MqttKit.h  module/mqtt/inc/
cp NET/MQTT/Common.h   module/mqtt/inc/
```

- [ ] **Step 2: Write `module/mqtt/demo_main.c`**

```c
/**
 * @file    demo_main.c
 * @brief   MQTT 协议封包 demo — 离线测试 CONNECT/PUBLISH 封包和 CONNACK 解包
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    本 demo 不需要 WiFi 或网络硬件，仅验证 MQTT 协议层编解码
 *          Keil 工程需开启堆 (Heap >= 0x400)，MqttKit 内部使用 malloc
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "MqttKit.h"

/* Print a byte buffer as hex string on UART1 */
static void print_hex(const char *label, const unsigned char *data, unsigned short len)
{
    unsigned short i;
    UsartPrintf(USART_DEBUG, "%s (%d bytes): ", label, len);
    for(i = 0; i < len; i++)
        UsartPrintf(USART_DEBUG, "%02X ", data[i]);
    UsartPrintf(USART_DEBUG, "\r\n");
}

int main(void)
{
    MQTT_PACKET_STRUCTURE pkt = {NULL, 0, 0, 0};

    /* Hardcoded CONNACK: type=CONNACK(0x20), len=2, sp=0, rc=0 (accepted) */
    const unsigned char connack_bytes[] = {0x20, 0x02, 0x00, 0x00};
    unsigned char connack_result = 0xFF;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[MQTT] Demo start (offline packet encode/decode test)\r\n");

    /* ---- Test 1: CONNECT packet ---- */
    UsartPrintf(USART_DEBUG, "\r\n[MQTT] Test 1: CONNECT packet\r\n");
    if(MQTT_PacketConnect("demo_product", "demo_token", "demo_client",
                          60, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &pkt) == 0)
    {
        print_hex("  CONNECT", pkt._data, pkt._len);
        UsartPrintf(USART_DEBUG, "  PASS: CONNECT encoded (%d bytes)\r\n", pkt._len);
        MQTT_DeleteBuffer(&pkt);
    }
    else
    {
        UsartPrintf(USART_DEBUG, "  FAIL: MQTT_PacketConnect returned error\r\n");
    }

    /* ---- Test 2: PUBLISH packet ---- */
    UsartPrintf(USART_DEBUG, "\r\n[MQTT] Test 2: PUBLISH packet (QoS 0)\r\n");
    {
        const char *topic   = "test/topic";
        const char *payload = "hello";
        if(MQTT_PacketPublish(10, topic, payload, 5,
                              MQTT_QOS_LEVEL0, 0, 1, &pkt) == 0)
        {
            print_hex("  PUBLISH", pkt._data, pkt._len);
            UsartPrintf(USART_DEBUG, "  PASS: PUBLISH encoded (%d bytes)\r\n", pkt._len);
            MQTT_DeleteBuffer(&pkt);
        }
        else
        {
            UsartPrintf(USART_DEBUG, "  FAIL: MQTT_PacketPublish returned error\r\n");
        }
    }

    /* ---- Test 3: CONNACK decode ---- */
    UsartPrintf(USART_DEBUG, "\r\n[MQTT] Test 3: CONNACK decode\r\n");
    print_hex("  CONNACK bytes", connack_bytes, sizeof(connack_bytes));
    if(MQTT_UnPacketRecv((unsigned char *)connack_bytes) == MQTT_PKT_CONNACK)
    {
        connack_result = MQTT_UnPacketConnectAck((unsigned char *)connack_bytes);
        UsartPrintf(USART_DEBUG, "  CONNACK return code: %d (%s)\r\n",
            connack_result, connack_result == 0 ? "ACCEPTED" : "REFUSED");
    }
    else
    {
        UsartPrintf(USART_DEBUG, "  FAIL: packet type is not CONNACK\r\n");
    }

    /* ---- Halt ---- */
    UsartPrintf(USART_DEBUG, "\r\n[MQTT] Demo complete\r\n");
    while(1) { }
}
```

- [ ] **Step 3: Create Keil project `module/mqtt/mqtt.uvprojx`**

Source groups:
- `Startup`, `Core`, `Fwlib` (standard)
- `Drivers`: `drivers/delay.c`, `drivers/usart.c`
- `Module`: `src/MqttKit.c`
- `App`: `demo_main.c`

Include paths: `core/;fwlib/inc/;drivers/;inc/`
**Heap size: 0x400** (required — MqttKit uses malloc)

- [ ] **Step 4: Write `module/mqtt/README.md`**

README 包含：
- **模块实现原理**：MQTT CONNECT 包结构（固定头 0x10 + 剩余长度 + 协议名 + 版本 + connect flags + keepalive + payload）；PUBLISH 包结构（0x30|qos + 剩余长度 + topic长度 + topic + packet_id(QoS>0) + payload）；CONNACK 包（0x20 0x02 sp rc）；`MQTT_MallocBuffer`/`MQTT_FreeBuffer` 映射到 `malloc`/`free`，需要 Keil 堆；QoS 0 无 PUBACK，QoS 1 需要 PUBACK；MQTT_PACKET_STRUCTURE 字段含义
- **Demo 使用指南**：无需硬件（离线测试）、Keil heap 设置方法、预期 UART 输出

- [ ] **Step 5: Commit**

```bash
git add module/mqtt/
git commit -m "[sensors] add module/mqtt demo"
```

---

## Task 6: module/onenet — OneNET 数据上传 Demo

**Files:**
- Create: `module/onenet/demo_main.c`
- Create: `module/onenet/src/onenet.c` (actuator stubs applied)
- Create: `module/onenet/src/esp8266.c` (OLED calls stripped, same as Task 4)
- Create: `module/onenet/src/MqttKit.c`, `hmac_sha1.c`, `base64.c`, `cJSON.c`
- Create: `module/onenet/inc/` (all headers + stub_actuators.h)
- Create: `module/onenet/drivers/`, `core/`, `fwlib/`
- Create: `module/onenet/onenet.uvprojx`
- Create: `module/onenet/README.md`

- [ ] **Step 1: Copy shared dependencies**

```bash
cp -r core  module/onenet/core
cp -r fwlib module/onenet/fwlib
mkdir -p module/onenet/drivers
cp hardware/src/delay.c  module/onenet/drivers/
cp hardware/inc/delay.h  module/onenet/drivers/
cp hardware/src/usart.c  module/onenet/drivers/
cp hardware/inc/usart.h  module/onenet/drivers/
```

- [ ] **Step 2: Copy all module source files**

```bash
mkdir -p module/onenet/src module/onenet/inc
cp NET/onenet/src/onenet.c      module/onenet/src/
cp NET/onenet/inc/onenet.h      module/onenet/inc/
cp NET/onenet/src/hmac_sha1.c   module/onenet/src/
cp NET/onenet/inc/hmac_sha1.h   module/onenet/inc/
cp NET/onenet/src/base64.c      module/onenet/src/
cp NET/onenet/inc/base64.h      module/onenet/inc/
cp NET/MQTT/MqttKit.c           module/onenet/src/
cp NET/MQTT/MqttKit.h           module/onenet/inc/
cp NET/MQTT/Common.h            module/onenet/inc/
cp NET/CJSON/cJSON.c            module/onenet/src/
cp NET/CJSON/cJSON.h            module/onenet/inc/
cp NET/device/src/esp8266.c     module/onenet/src/
cp NET/device/inc/esp8266.h     module/onenet/inc/
```

- [ ] **Step 3: Create `module/onenet/inc/stub_actuators.h`**

```c
/**
 * @file    stub_actuators.h
 * @brief   Actuator stubs for onenet demo — replaces beep/fan/led/mq dependencies
 *
 * onenet.c references hardware actuator symbols (Beep_Status, Fan_Set, Led_Set,
 * Mq2_GetPercentage, etc.). This header provides no-op stubs so the module
 * compiles without the actuator drivers.
 */

#ifndef STUB_ACTUATORS_H
#define STUB_ACTUATORS_H

#include "stm32f10x.h"

/* Beep */
#define Beep_Status  0
#define BEEP_ON      1
#define BEEP_OFF     0
#define Beep_Set(x)  do {} while(0)

/* Fan */
#define FAN_ON       1
#define FAN_OFF      0
#define Fan_Set(x)   do {} while(0)

/* LED */
#define LED_ON       1
#define LED_OFF      0
#define Led_Set(x)   do {} while(0)

/* MQ sensors — return 0 (no gas detected) */
static inline uint8_t Mq2_GetPercentage(void) { return 0; }
static inline uint8_t Mq3_GetPercentage(void) { return 0; }
static inline uint8_t Mq4_GetPercentage(void) { return 0; }
static inline uint8_t Mq7_GetPercentage(void) { return 0; }

#endif /* STUB_ACTUATORS_H */
```

- [ ] **Step 4: Patch `module/onenet/src/onenet.c`**

Make these two changes to the copied `onenet.c`:

1. Remove the three actuator includes and add stub header:
```c
/* MODULE CHANGE: beep.h/fan.h/mq.h replaced with stub_actuators.h for portability */
// #include "beep.h"    /* removed */
// #include "fan.h"     /* removed */
// #include "mq.h"      /* removed */
#include "stub_actuators.h"
```

2. The `extern` declarations for globals at line 381 reference `main.c` globals.
In the demo, these will be defined in `demo_main.c`. Confirm the externs match the demo globals (see Step 5).

- [ ] **Step 5: Patch `module/onenet/src/esp8266.c`**

Same OLED-stripping change as Task 4, Step 2. Replace OLED calls in `ESP8266_Init()` with `UsartPrintf` and add `/* MODULE CHANGE */` comment.

- [ ] **Step 6: Write `module/onenet/demo_main.c`**

```c
/**
 * @file    demo_main.c
 * @brief   OneNET demo — 完整上传链路验证: WiFi→TCP→MQTT鉴权→发布一条数据
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    ESP8266 TX→PA3(USART2 RX), ESP8266 RX→PA2(USART2 TX)
 *          UART1 TX→PA9 (115200 baud, 调试输出)
 * @note    上传前必须修改以下配置：
 *          1. src/onenet.c: PROID, ACCESS_KEY, DEVICE_NAME
 *          2. src/esp8266.c: ESP8266_WIFI_INFO (SSID + 密码)
 */

#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "esp8266.h"
#include "onenet.h"

#define ESP8266_ONENET_INFO  "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"

/* Demo sensor values — hardcoded, not from real sensors */
u8  temp      = 25;    /* 温度, °C   */
u8  humi      = 60;    /* 湿度, %    */
int light_pct = 45;    /* 光照, 0-99 */
int mq2_vol   = 0;     /* MQ2, %     */

/* Thresholds (required by onenet.c externs) */
u8  temp_adj = 37;
u8  humi_adj = 90;
int light_th = 30;
int smog_th  = 50;

/* Actuator mode flags (required by onenet.c externs) */
u8  fan_mode = 1;
u8  led_mode = 1;

int main(void)
{
    unsigned char *dataPtr = NULL;
    unsigned char retry    = 0;

    /* ---- Init ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);

    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Demo start\r\n");
    UsartPrintf(USART_DEBUG, "[OneNET] Demo data: temp=%d, humi=%d, light=%d\r\n",
                temp, humi, light_pct);

    /* ---- Step 1: WiFi connect ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 1: ESP8266 WiFi init\r\n");
    ESP8266_Init();   /* blocks until WiFi connected */

    /* ---- Step 2: TCP connect to OneNET ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 2: TCP connect to mqtts.heclouds.com:1883\r\n");
    retry = 0;
    while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))   /* signature: (char *cmd, char *res) */
    {
        if(++retry > 5)
        {
            UsartPrintf(USART_DEBUG, "[OneNET] ERR: TCP connect failed after 5 retries\r\n");
            while(1) { }
        }
        UsartPrintf(USART_DEBUG, "[OneNET] TCP retry %d...\r\n", retry);
        DelayXms(1000);
    }
    UsartPrintf(USART_DEBUG, "[OneNET] TCP connected\r\n");

    /* ---- Step 3: MQTT DevLink (CONNECT + CONNACK) ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 3: MQTT DevLink (HMAC-SHA1 auth)\r\n");
    retry = 0;
    while(OneNet_DevLink())
    {
        if(++retry > 5)
        {
            UsartPrintf(USART_DEBUG, "[OneNET] ERR: DevLink failed after 5 retries\r\n");
            while(1) { }
        }
        ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT");   /* 2-arg signature */
        DelayXms(500);
    }
    UsartPrintf(USART_DEBUG, "[OneNET] MQTT connected (CONNACK = 0)\r\n");

    /* ---- Step 4: Subscribe ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 4: Subscribe to property/set\r\n");
    OneNET_Subscribe();
    DelayXms(500);
    dataPtr = ESP8266_GetIPD(0);
    if(dataPtr != NULL) OneNet_RevPro(dataPtr);

    /* ---- Step 5: Publish sensor data ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 5: Publish demo data\r\n");
    OneNet_SendData();
    UsartPrintf(USART_DEBUG, "[OneNET] Publish sent\r\n");

    /* ---- Step 6: Poll for cloud response ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Step 6: Poll cloud response (2s)\r\n");
    DelayXms(2000);
    dataPtr = ESP8266_GetIPD(0);
    if(dataPtr != NULL)
    {
        UsartPrintf(USART_DEBUG, "[OneNET] Cloud response received\r\n");
        OneNet_RevPro(dataPtr);
    }
    else
    {
        UsartPrintf(USART_DEBUG, "[OneNET] No cloud response (normal for QoS 0)\r\n");
    }

    /* ---- Halt ---- */
    UsartPrintf(USART_DEBUG, "\r\n[OneNET] Demo complete\r\n");
    ESP8266_Clear();
    while(1) { }
}
```

- [ ] **Step 7: Create Keil project `module/onenet/onenet.uvprojx`**

Source groups:
- `Startup`: `core/startup/arm/startup_stm32f10x_md.s`
- `Core`: `core/system_stm32f10x.c`
- `Fwlib`: all `.c` in `fwlib/src/`
- `Drivers`: `drivers/delay.c`, `drivers/usart.c`
- `Module`: `src/onenet.c`, `src/esp8266.c`, `src/MqttKit.c`, `src/hmac_sha1.c`, `src/base64.c`, `src/cJSON.c`
- `App`: `demo_main.c`

Include paths: `core/;fwlib/inc/;drivers/;inc/`
Heap: 0x400

- [ ] **Step 8: Write `module/onenet/README.md`**

This is the most detailed README. Include:

**模块实现原理** — full pipeline walkthrough:

```
传感器数据 → JSON 组包 → MQTT PUBLISH 封包 → ESP8266 AT+CIPSEND → TCP → OneNET云平台
```

Sections to cover:
1. 整体数据流架构图（文字 ASCII art）
2. 第一层：HMAC-SHA1 鉴权令牌生成（`OneNET_Authorization`）— Access Key Base64解码 → HMAC-SHA1 → Base64编码 → URL编码 → 组成 token 字符串
3. 第二层：MQTT CONNECT 握手（`OneNet_DevLink`）— MQTT_PacketConnect → ESP8266_SendData → 等待 CONNACK → MQTT_UnPacketConnectAck 返回码含义
4. 第三层：订阅下行主题（`OneNET_Subscribe`）— topic 格式 `$sys/{PROID}/{DEVICE}/thing/property/set`
5. 第四层：JSON 数据组包（`OneNet_FillBuf`）— 物模型 JSON 格式 `{"id":"123","params":{"temp":{"value":25},...}}`；`#ifdef UPLOAD_MQ_DATA` 控制 MQ 字段；`Beep_Status` 使用 stub 替换
6. 第五层：MQTT PUBLISH 发送（`OneNet_SendData`）— `MQTT_PacketSaveData` 封包 → `ESP8266_SendData` 发送 → `AT+CIPSEND=N` 流程
7. 第六层：云端下行命令处理（`OneNet_RevPro`）— cJSON 解析 `params` 字段；fan/led/temp_adj/humi_adj/light_th 更新；mode=3 覆盖逻辑
8. 关键设计要点（volatile ISR buffer、HMAC 密钥 Base64 解码、stub 设计原因）

**Demo 使用指南**：配置项（PROID/ACCESS_KEY/DEVICE_NAME + WiFi）、接线表、预期串口输出（每步骤）、复用说明

- [ ] **Step 9: Commit**

```bash
git add module/onenet/
git commit -m "[sensors] add module/onenet demo"
```

---

## Notes for Implementer

1. **Keil `.uvprojx` files are XML.** They can be generated by creating a new project in Keil and manually adding the source groups and files listed above. The include paths are set in Options → C/C++ → Include Paths.

2. **Heap setting:** In Keil Options → Target → "Code Generation" area, set Heap Size to `0x400` for the `mqtt` and `onenet` projects.

3. **No `stm32f10x_it.c`** in any project. The startup file's `[WEAK]` handler declarations are overridden by the strong `USART2_IRQHandler` in `esp8266.c`.

4. **`sys.h` / `sys.c`** may be referenced by `hardware/inc/` — check and copy to `drivers/` if any module src file includes it.

5. **onenet.c extern globals** (`temp`, `humi`, `light_pct`, `mq2_vol`, `fan_mode`, `led_mode`, `temp_adj`, `humi_adj`, `light_th`, `smog_th`) must be defined in `demo_main.c` — they are declared `extern` in `onenet.c:381-383`.

6. **Each module is independent.** Do not create symlinks or references between module directories.
