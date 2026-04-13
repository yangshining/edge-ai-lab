# DHT11 温湿度传感器模块

## A. 模块实现原理

### 1. 功能说明

DHT11 是一款单总线数字温湿度传感器，通过一根数据线（DATA）与主控芯片通信。传感器内部集成了温湿度测量单元和 ADC，每次通信返回 40 位数据（湿度整数 + 湿度小数 + 温度整数 + 温度小数 + 校验和），测量范围为温度 0~50°C、湿度 20%~90%RH，精度分别为 ±2°C 和 ±5%RH，适用于室内环境监测场景。

---

### 2. 整体架构

DHT11 采用单总线半双工通信协议，一次完整的读数时序如下：

```
MCU 拉低 DATA ≥18ms（复位信号）
  └→ MCU 拉高 DATA 20~40µs（释放总线）
       └→ DHT11 拉低 DATA 40~80µs（应答低）
            └→ DHT11 拉高 DATA 40~80µs（应答高）
                 └→ DHT11 连续发送 40 bit 数据
                      └→ MCU 逐位采样，完成读取
```

MCU 的数据引脚在发送复位脉冲时配置为推挽输出，接收数据时切换为输入模式，通过 `DHT11_IO_OUT()` / `DHT11_IO_IN()` 两个宏直接操作 `GPIOA->CRL` 寄存器完成模式切换。

---

### 3. 核心实现流程

#### 3.1 复位与应答检测

`DHT11_Rst()` 向传感器发送起始信号，`DHT11_Check()` 验证传感器是否在线：

```c
void DHT11_Rst(void)
{
    DHT11_IO_OUT();      // 切换为输出模式
    DHT11_DQ_OUT(0);     // 拉低 DATA
    DelayXms(20);        // 保持低电平至少 18ms
    DHT11_DQ_OUT(1);     // 释放总线（拉高）
    DelayUs(30);         // 等待 20~40µs
}

uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    DHT11_IO_IN();       // 切换为输入模式
    // 等待 DHT11 拉低（应答低，40~80µs）
    while (DHT11_DQ_IN && retry < 100) { retry++; DelayUs(1); }
    if (retry >= 100) return 1;   // 超时：传感器不存在
    retry = 0;
    // 等待 DHT11 再次拉高（应答高，40~80µs）
    while (!DHT11_DQ_IN && retry < 100) { retry++; DelayUs(1); }
    if (retry >= 100) return 1;   // 超时：应答异常
    return 0;                     // 应答正常
}
```

#### 3.2 位读取原理

DHT11 用高电平持续时间区分 0 和 1：

- 每 bit 以约 50µs 低电平起始；
- 随后拉高：**26~28µs → bit = 0**，**70µs → bit = 1**；
- 在低变高之后延迟 **40µs** 采样，若引脚仍为高则为 1，否则为 0。

```c
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;
    while (DHT11_DQ_IN  && retry < 100) { retry++; DelayUs(1); } // 等待低电平起始
    retry = 0;
    while (!DHT11_DQ_IN && retry < 100) { retry++; DelayUs(1); } // 等待拉高
    DelayUs(40);             // 延迟 40µs 后采样
    return DHT11_DQ_IN ? 1 : 0;
}
```

#### 3.3 40-bit 帧格式

| 字节索引 | 内容         | 说明                    |
|----------|--------------|-------------------------|
| `buf[0]` | 湿度整数部分 | 单位 %RH                |
| `buf[1]` | 湿度小数部分 | DHT11 固定为 0          |
| `buf[2]` | 温度整数部分 | 单位 °C                 |
| `buf[3]` | 温度小数部分 | DHT11 固定为 0          |
| `buf[4]` | 校验和       | = buf[0]+buf[1]+buf[2]+buf[3] 的低 8 位 |

#### 3.4 完整读取流程（含校验）

```c
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5], i;
    DHT11_Rst();
    if (DHT11_Check() == 0)
    {
        for (i = 0; i < 5; i++)
            buf[i] = DHT11_Read_Byte();

        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
        else return 2;   // 校验和不匹配
    }
    else return 1;       // 传感器未响应
    return 0;            // 读取成功
}
```

校验失败（返回 2）通常是时序受干扰所致，上层应重试或丢弃该帧。

---

### 4. 关键设计要点

1. **PA0 与注释不符**：`dht11.h` 文件顶部注释写的是 `PA11`，但宏实际操作的是 `GPIO_Pin_0`（即 **PA0**）。部署前需确认硬件连线接 PA0，而非 PA11：
   ```c
   // dht11.h 中的实际宏定义（操作 PA0）
   #define DHT11_IO_IN()  {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=8;}
   #define DHT11_IO_OUT() {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=3;}
   #define DHT11_DQ_OUT(X) GPIO_WriteBit(GPIOA, GPIO_Pin_0, X)
   #define DHT11_DQ_IN     GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)
   ```

2. **上拉电阻要求**：单总线协议要求 DATA 线通过 4.7kΩ 上拉电阻连接至 VCC（3.3V 或 5V）。`DHT11_Init()` 将 PA0 配置为推挽输出（`GPIO_Mode_Out_PP`），输入采样阶段依赖外部上拉，请勿省略上拉电阻。

3. **时序依赖 `Delay_Init()`**：所有 us/ms 延迟函数均依赖 `Delay_Init()` 初始化 SysTick，必须在 `DHT11_Init()` 之前调用，否则时序错误导致读取失败。

---

## B. Demo 使用指南

### 1. 引脚配置

| 信号        | MCU 引脚 | 说明                              |
|-------------|----------|-----------------------------------|
| DHT11 DATA  | PA0      | 需外接 4.7kΩ 上拉电阻至 VCC      |
| DHT11 VCC   | 3.3V/5V  | 传感器供电                        |
| DHT11 GND   | GND      | 共地                              |
| UART1 TX    | PA9      | 115200 baud，连接 USB-TTL 调试器  |
| UART1 RX    | PA10     | （本 demo 不使用接收）             |

### 2. Demo 行为

上电后，串口将持续每隔 1 秒输出一行读数：

```
[DHT11] Demo start
[DHT11] Pin: PA0, Interval: 1000 ms
[DHT11] Init OK
[DHT11] Temp: 25 C, Humi: 60 %
[DHT11] Temp: 25 C, Humi: 61 %
[DHT11] Temp: 26 C, Humi: 60 %
```

若传感器未接或接线错误，输出：

```
[DHT11] Read failed (code 1)
```

若读取到校验和错误，输出：

```
[DHT11] Read failed (code 2)
```

### 3. 编译与烧录步骤

1. 用 **Keil MDK-ARM V5** 打开 `dht11.uvprojx`；
2. 按 **F7** 编译，输出文件位于 `output/dht11.hex`；
3. 用 ST-Link / J-Link 连接目标板，点击 **Download** 烧录；
4. 打开串口助手（115200 8N1），按下复位键，观察输出。

### 4. 配置项

| 宏定义              | 文件           | 默认值 | 说明             |
|---------------------|----------------|--------|------------------|
| `READ_INTERVAL_MS`  | `demo_main.c`  | 1000   | 读取间隔（ms）   |

> DHT11 规格书要求两次读取间隔不短于 1s，请勿将 `READ_INTERVAL_MS` 设置为小于 1000 的值。

### 5. 复用说明

将本模块集成到其他工程只需复制以下文件：

```
src/dht11.c
inc/dht11.h
```

并确保工程中已包含 `delay.h` / `delay.c`（提供 `DelayUs`、`DelayXms`）。

调用方式：

```c
#include "dht11.h"

// 初始化（需在 Delay_Init() 之后调用）
// DHT11_Init() returns 0 if sensor is present, 1 if absent
// In production code, check the return value:
if (DHT11_Init() != 0) {
    // sensor not detected — handle error
}

// 循环读取
uint8_t temp, humi;
if (DHT11_Read_Data(&temp, &humi) == 0)
{
    // temp: 温度 (°C), humi: 湿度 (%RH)
}
```

头文件无需修改任何配置项，引脚固定为 **PA0**。若需迁移到其他引脚，修改 `dht11.h` 中的四个宏（`DHT11_IO_IN`、`DHT11_IO_OUT`、`DHT11_DQ_OUT`、`DHT11_DQ_IN`）即可。
