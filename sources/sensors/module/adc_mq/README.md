# adc_mq — ADC + MQ 气体传感器模块 Demo

STM32F103C8T6 平台，ADC1 + DMA1 五通道连续采集 MQ 系列气体传感器及光照传感器，每秒通过 UART1 打印原始 ADC 值和百分比。

---

## A. 模块实现原理

### 1. 功能说明

本模块使用 **ADC1 + DMA1 通道1** 对以下 5 路模拟信号进行连续循环采集：

| 传感器 | 引脚 | ADC 通道 | 检测对象 |
|--------|------|----------|----------|
| MQ3    | PA0  | Channel_0 | 酒精 (Alcohol) |
| MQ4    | PA1  | Channel_1 | 甲烷/天然气 (Methane/CNG) |
| 光照   | PA4  | Channel_4 | 光照强度 (Luminance) |
| MQ2    | PA5  | Channel_5 | 烟雾/LPG/CO (Smoke) |
| MQ7    | PA6  | Channel_6 | 一氧化碳 (Carbon Monoxide) |

> PA2/PA3 已被 USART2（ESP8266 TX/RX）占用，MQ5（PA2）和 MQ6（PA3）在本硬件配置中**不可用**，对应函数始终返回 0。

### 2. 整体架构

```
ADC1（连续扫描 5 通道）
    │  每完成一轮转换，DMA 自动搬运结果
    ▼
DMA1 Channel1（循环模式，CPU 零负担）
    │
    ▼
ADC_ConvertedValue[5]  （__IO uint16_t，全局缓冲区）
    │
    ├── [ADC_IDX_MQ3=0]   PA0  MQ3  Alcohol
    ├── [ADC_IDX_MQ4=1]   PA1  MQ4  Methane
    ├── [ADC_IDX_LIGHT=2] PA4  Light sensor
    ├── [ADC_IDX_MQ2=3]   PA5  MQ2  Smoke
    └── [ADC_IDX_MQ7=4]   PA6  MQ7  CO

主循环任意时刻读取 ADC_ConvertedValue[] 即为最新值（无需等待、无锁）
```

### 3. 核心实现流程

#### 3.1 ADC 时钟配置顺序（关键）

```c
RCC_ADCCLKConfig(RCC_PCLK2_Div8);  /* 必须在 ADC_Init 之前调用 */
ADC_Init(ADC_x, &ADC_InitStructure);
```

**原因：** STM32F103 ADC 时钟上限为 14 MHz。系统主频 72 MHz，PCLK2 默认亦为 72 MHz。复位后 ADC 预分频默认值可能导致 ADC 时钟达到 36 MHz，违反规范，造成采样错误或转换结果异常。`RCC_ADCCLKConfig(RCC_PCLK2_Div8)` 将 ADC 时钟设为 72÷8 = **9 MHz**，满足规范。若在 `ADC_Init` 之后再调用此函数，ADC 已按错误时钟初始化，行为未定义。

#### 3.2 DMA 连续循环模式配置

```c
DMA_InitStructure.DMA_DIR    = DMA_DIR_PeripheralSRC;   /* 外设 → 内存 */
DMA_InitStructure.DMA_Mode   = DMA_Mode_Circular;        /* 循环模式，自动重装计数器 */
DMA_InitStructure.DMA_BufferSize = NOFCHANEL;            /* 5 个半字 */
```

`DMA_Mode_Circular` 使 DMA 在传完 5 个半字后自动将地址指针和计数器复位，下一轮 ADC 转换结束后继续填充，无需 CPU 干预。

#### 3.3 ADC Rank → 宏 → 引脚映射

| Rank | DMA 索引宏 | 引脚 | ADC 通道 | 传感器 |
|------|-----------|------|----------|--------|
| 1 | `ADC_IDX_MQ3`   = 0 | PA0 | Channel_0 | MQ3 酒精 |
| 2 | `ADC_IDX_MQ4`   = 1 | PA1 | Channel_1 | MQ4 甲烷 |
| 3 | `ADC_IDX_LIGHT` = 2 | PA4 | Channel_4 | 光照传感器 |
| 4 | `ADC_IDX_MQ2`   = 3 | PA5 | Channel_5 | MQ2 烟雾 |
| 5 | `ADC_IDX_MQ7`   = 4 | PA6 | Channel_6 | MQ7 CO |

Rank 与宏的关系为 `rank = ADC_IDX_* + 1`，在 `ADC_RegularChannelConfig` 中保持一致：

```c
ADC_RegularChannelConfig(ADC_x, ADC_Channel_0, ADC_IDX_MQ3   + 1, ADC_SampleTime_55Cycles5);
ADC_RegularChannelConfig(ADC_x, ADC_Channel_1, ADC_IDX_MQ4   + 1, ADC_SampleTime_55Cycles5);
ADC_RegularChannelConfig(ADC_x, ADC_Channel_4, ADC_IDX_LIGHT + 1, ADC_SampleTime_55Cycles5);
ADC_RegularChannelConfig(ADC_x, ADC_Channel_5, ADC_IDX_MQ2   + 1, ADC_SampleTime_55Cycles5);
ADC_RegularChannelConfig(ADC_x, ADC_Channel_6, ADC_IDX_MQ7   + 1, ADC_SampleTime_55Cycles5);
```

#### 3.4 原始值转百分比

`mq.c` 中使用整数除法，上限截断为 100%：

```c
static uint8_t adc_to_pct(uint16_t v)
{
    uint32_t p = (uint32_t)v * 100u / 4095u;
    return (uint8_t)(p > 100u ? 100u : p);
}
```

光照传感器在 `demo_main.c` 中使用浮点计算以保留精度：

```c
(int)(ADC_ConvertedValue[ADC_IDX_LIGHT] / 4095.0f * 100)
```

两种方式结果相同，整数版本更适合资源受限场景。

### 4. 关键设计要点

1. **`RCC_ADCCLKConfig` 顺序**：必须在 `ADC_Init` 之前调用，否则 ADC 以 36 MHz 超频时钟初始化，采样结果不可信。
2. **DMA Circular 模式**：一次配置，永久自动刷新缓冲区，主循环直接读取无需轮询标志位。
3. **`ADC_IDX_*` 宏不可替换为裸索引**：Rank 顺序与数组下标强绑定，若手动写数字，后续调整通道顺序时极易引入 bug。
4. **MQ5/MQ6 不可用**：PA2/PA3 硬件上已复用为 USART2，不得在 ADC 中配置这两个引脚。

---

## B. Demo 使用指南

### 1. 引脚配置表

| 信号 | 引脚 | 方向 | 说明 |
|------|------|------|------|
| MQ3 模拟输出 | PA0 | 输入 (AIN) | 酒精传感器 |
| MQ4 模拟输出 | PA1 | 输入 (AIN) | 甲烷传感器 |
| 光照传感器模拟输出 | PA4 | 输入 (AIN) | 光敏电阻分压 |
| MQ2 模拟输出 | PA5 | 输入 (AIN) | 烟雾传感器 |
| MQ7 模拟输出 | PA6 | 输入 (AIN) | CO 传感器 |
| UART1 TX (调试) | PA9 | 输出 | 115200 baud，接 USB-TTL |
| UART1 RX | PA10 | 输入 | 可选，接收 PC 端输入 |

> MQ 传感器供电 5V，模拟输出分压后接入 STM32 3.3V ADC 输入（注意不超过 3.3V）。

### 2. Demo 行为

上电后每 **1 秒**循环打印一组 5 行数据，格式示例：

```
[ADC_MQ] Demo start
[ADC_MQ] 5-channel DMA ADC: MQ3(PA0) MQ4(PA1) LIGHT(PA4) MQ2(PA5) MQ7(PA6)
[ADC_MQ] ADC+DMA init OK

[ADC_MQ] MQ3 (PA0) Alcohol  : raw=1024   25%
[ADC_MQ] MQ4 (PA1) Methane  : raw= 512   12%
[ADC_MQ] LIGHT(PA4) Luminance: raw=2048   50%
[ADC_MQ] MQ2 (PA5) Smoke    : raw= 205    5%
[ADC_MQ] MQ7 (PA6) CO       : raw= 307    7%

[ADC_MQ] MQ3 (PA0) Alcohol  : raw=1031   25%
...
```

- `raw`：12 位 ADC 原始值，范围 0–4095
- 百分比：`raw * 100 / 4095`，范围 0–100%

### 3. 编译与烧录

1. 用 **Keil MDK-ARM V5** 打开 `adc_mq.uvprojx`
2. 按 **F7** 编译，输出文件位于 `output/adc_mq.hex`
3. 通过 ST-Link 连接目标板，点击 **Download** 烧录
4. 打开串口工具（115200 8N1），复位开发板，观察输出

### 4. 配置项

| 宏 | 位置 | 默认值 | 说明 |
|----|------|--------|------|
| `PRINT_INTERVAL_MS` | `demo_main.c` | 1000 | 打印间隔（毫秒） |
| `NOFCHANEL` | `inc/adc.h` | 5 | ADC 通道数，勿修改 |
| `ADC_IDX_*` | `inc/adc.h` | 0–4 | DMA 缓冲区索引宏，勿修改 |

### 5. 复用说明

将本模块集成到其他项目时：

1. 复制 `src/adc.c`、`src/mq.c`、`inc/adc.h`、`inc/mq.h` 到目标项目
2. 在初始化阶段调用 `ADCx_Init()`，等待约 100 ms 让 DMA 填充首批数据
3. 之后任意时刻通过 `ADC_ConvertedValue[ADC_IDX_*]` 读取原始值，或调用 `MqX_GetPercentage()` 获取百分比
4. 确保 `RCC_ADCCLKConfig(RCC_PCLK2_Div8)` 在 `ADC_Init` 之前执行（已在 `ADCx_Mode_Config` 中保证）
5. **不要**在目标项目中再次配置 PA2/PA3 为 ADC 输入（已被 USART2 占用）
