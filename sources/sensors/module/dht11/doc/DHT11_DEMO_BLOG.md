# STM32 DHT11 Demo 实现拆解：温湿度采集、OLED 显示与软件 I2C

## 摘要

这篇文章拆解 `module/dht11` 里的完整 demo。工程包含 3 条主线：

- STM32F103 按 DHT11 的单总线时序读取温湿度
- 通过 UART1 输出调试信息
- 通过 SSD1306 OLED 显示结果，OLED 总线由 GPIO 模拟 I2C 实现

文中所有分析都对应工程里的真实代码，重点放在时序、数据流和实现细节。

## 目录

- [1. 工程目标与硬件连接](#1-工程目标与硬件连接)
- [2. 主流程](#2-主流程)
- [3. DHT11 读取原理](#3-dht11-读取原理)
- [4. DHT11 驱动代码拆解](#4-dht11-驱动代码拆解)
- [5. 为什么要先初始化延时模块](#5-为什么要先初始化延时模块)
- [6. OLED 与软件 I2C](#6-oled-与软件-i2c)
- [7. SSD1306 命令与数据写入](#7-ssd1306-命令与数据写入)
- [8. OLED 初始化过程](#8-oled-初始化过程)
- [9. OLED 文本和数字显示](#9-oled-文本和数字显示)
- [10. 主循环如何组织显示逻辑](#10-主循环如何组织显示逻辑)
- [11. 串口输出的作用](#11-串口输出的作用)
- [12. 代码里的几个工程细节](#12-代码里的几个工程细节)
- [13. 可以继续改进的点](#13-可以继续改进的点)
- [14. 总结](#14-总结)

## 1. 工程目标与硬件连接

这个 demo 做的事情很直接：每隔 1 秒读取一次 DHT11，把结果同时输出到串口和 OLED。

`demo_main.c` 文件头部给出了引脚定义：

```c
 * @pins    DHT11 DATA -> PA0
 *          OLED SCL  -> PB1
 *          OLED SDA  -> PB0
 *          UART1 TX  -> PA9
```

对应关系如下：

1. `PA0` 接 DHT11 数据线
2. `PB1` 作为 OLED 的 SCL
3. `PB0` 作为 OLED 的 SDA
4. `PA9` 作为 UART1 TX，用于串口输出

目标板是 `STM32F103C8T6`，主频 72 MHz。

涉及的主要文件：

- `module/dht11/demo_main.c`
- `module/dht11/inc/dht11.h`
- `module/dht11/src/dht11.c`
- `module/dht11/inc/oled.h`
- `module/dht11/src/oled.c`

## 2. 主流程

主函数很短，但已经把整个 demo 的结构写清楚了：

```c
int main(void)
{
    uint8_t temp = 0, humi = 0;
    uint8_t ret  = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);
    OLED_Init();
    OLED_Clear();

    DHT11_Init();

    OLED_ShowString(0, 0, (u8 *)"DHT11 Demo", 16);
    OLED_ShowString(0, 2, (u8 *)"Temp:     C", 16);
    OLED_ShowString(0, 4, (u8 *)"Humi:     %", 16);
    OLED_ShowString(0, 6, (u8 *)"Status: Init", 16);

    while(1)
    {
        ret = DHT11_Read_Data(&temp, &humi);

        if(ret == 0)
        {
            OLED_ShowNum(48, 2, temp, 3, 16);
            OLED_ShowNum(48, 4, humi, 3, 16);
            OLED_ShowString(0, 6, (u8 *)"Status: OK  ", 16);
        }
        else
        {
            OLED_ShowString(48, 2, (u8 *)"---", 16);
            OLED_ShowString(48, 4, (u8 *)"---", 16);
        }

        DelayXms(READ_INTERVAL_MS);
    }
}
```

执行顺序可以概括成：

```text
上电
  -> 初始化 SysTick 延时
  -> 初始化串口
  -> 初始化 OLED
  -> 初始化 DHT11
  -> 绘制静态界面
  -> 循环:
       读取 DHT11
       更新 OLED
       输出串口
       延时 1 秒
```

这一层先抓住一个结论：主循环不复杂，难点不在流程控制，而在两个底层驱动。

- DHT11 依赖严格的微秒级时序
- OLED 依赖软件 I2C 正确产生总线波形

## 3. DHT11 读取原理

DHT11 不是模拟量传感器，不走 ADC。它内部完成采样和转换，然后通过单总线协议把数据发给 MCU。

一帧数据总共 40 bit，也就是 5 个字节：

```text
8bit 湿度整数
8bit 湿度小数
8bit 温度整数
8bit 温度小数
8bit 校验和
```

这个 demo 实际使用的是整数部分：

```c
*humi = buf[0];
*temp = buf[2];
```

DHT11 读取流程分 4 步：

1. 主机拉低数据线至少 18 ms，发送起始信号
2. 主机释放总线，等待 DHT11 应答
3. DHT11 依次输出 40 bit 数据
4. 主机完成校验和判断

判断 `0` 和 `1` 的关键不在电平值，而在高电平持续时间：

- 高电平较短，表示 `0`
- 高电平较长，表示 `1`

这份代码没有去精确测宽，而是在固定延时后采样，这也是 DHT11 驱动里非常常见的做法。

## 4. DHT11 驱动代码拆解

### 4.1 GPIO 模式切换

`dht11.h` 里定义了两个宏：

```c
#define DHT11_IO_IN()  {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=8;}
#define DHT11_IO_OUT() {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=3;}
```

原因很简单：同一根线上既要发送起始信号，也要接收传感器数据。

- 发起始信号时，PA0 是输出
- 等待应答和读数据时，PA0 是输入

这就是单总线场景下最典型的 GPIO 复用方式。

### 4.2 起始信号

`DHT11_Rst()` 对应主机发起读取请求：

```c
void DHT11_Rst(void)
{
    DHT11_IO_OUT();
    DHT11_DQ_OUT(0);
    DelayXms(20);
    DHT11_DQ_OUT(1);
    DelayUs(30);
}
```

这里有 3 个关键点：

1. 先切换成输出模式
2. 低电平保持 20 ms，满足 DHT11 至少 18 ms 的要求
3. 再拉高并等待 30 us，交出总线控制权

### 4.3 应答检测

`DHT11_Check()` 负责确认 DHT11 是否在线：

```c
uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    DHT11_IO_IN();

    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        DelayUs(1);
    }
    if (retry >= 100) return 1;

    retry = 0;
    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        DelayUs(1);
    }
    if (retry >= 100) return 1;

    return 0;
}
```

逻辑是：

- 先等 DHT11 拉低
- 再等 DHT11 拉高
- 两段都等到了，认为设备存在

`retry < 100` 是超时保护，避免一直卡死在等待里。

### 4.4 bit 读取

`DHT11_Read_Bit()` 是整个驱动的核心：

```c
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;
    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        DelayUs(1);
    }
    retry = 0;
    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        DelayUs(1);
    }
    DelayUs(40);
    if (DHT11_DQ_IN) return 1;
    else return 0;
}
```

这里的思路是：

1. 等待每一位开始前的低电平结束
2. 等总线变高
3. 延时 40 us 后采样

如果 40 us 时仍然是高电平，通常判断为 `1`；否则判断为 `0`。

这不是唯一写法，但在 DHT11 上足够常见，也足够实用。

### 4.5 字节读取与校验

一个字节由 8 个 bit 拼出来：

```c
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat;
    dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}
```

完整读取函数如下：

```c
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    DHT11_Rst();
    if (DHT11_Check() == 0)
    {
        for (i = 0; i < 5; i++)
        {
            buf[i] = DHT11_Read_Byte();
        }
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
        else return 2;
    }
    else return 1;
    return 0;
}
```

返回值设计得很清楚：

- `0`：成功
- `1`：无应答
- `2`：校验和错误

主循环只要判断这个返回值，就能决定 OLED 和串口应该显示什么。

## 5. 为什么要先初始化延时模块

主函数初始化顺序是：

```c
Delay_Init();
Usart1_Init(115200);
OLED_Init();
DHT11_Init();
```

`Delay_Init()` 必须靠前，原因有两个：

1. DHT11 时序依赖微秒级延时
2. OLED 初始化里也用到了延时

`drivers/delay.c` 里基于 `SysTick` 实现了 `DelayUs()`、`DelayXms()` 和 `DelayMs()`。例如：

```c
SysTick->LOAD = us * UsCount;
SysTick->VAL = 0;
SysTick->CTRL = 1;
```

这个 demo 的所有时序基础都建立在 `SysTick` 上。延时模块没准备好，DHT11 和 OLED 都可能出问题。

## 6. OLED 与软件 I2C

### 6.1 SSD1306 是单色屏

这里用的是 SSD1306 控制器的 0.96 寸 OLED。它是单色屏，不支持 RGB。

像素状态只有两种：

- 亮
- 灭

所以代码里并不存在“字体颜色切换”，本质是在写单色点阵显存。

### 6.2 这里走的不是硬件 I2C

`oled.h` 里直接把 GPIO 操作定义成了 I2C 信号线：

```c
#define OLED_SCLK_Clr()  GPIO_ResetBits(GPIOB, GPIO_Pin_1)
#define OLED_SCLK_Set()  GPIO_SetBits(GPIOB, GPIO_Pin_1)
#define OLED_SDIN_Clr()  GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define OLED_SDIN_Set()  GPIO_SetBits(GPIOB, GPIO_Pin_0)
```

对应关系如下：

- `PB1` -> SCL
- `PB0` -> SDA

这说明 OLED 驱动没有调用 STM32 硬件 I2C 外设，而是通过手动拉高拉低 GPIO 来模拟总线时序。

这类写法通常叫：

- 软件 I2C
- 模拟 I2C
- bit-banging I2C

## 7. SSD1306 命令与数据写入

### 7.1 Start 和 Stop

I2C 的起始条件是 `SCL 高电平时 SDA 由高变低`，停止条件是 `SCL 高电平时 SDA 由低变高`。

代码分别对应：

```c
void IIC_Start(void)
{
    OLED_SCLK_Set();
    OLED_SDIN_Set();
    OLED_SDIN_Clr();
    OLED_SCLK_Clr();
}
```

```c
void IIC_Stop(void)
{
    OLED_SCLK_Set();
    OLED_SDIN_Clr();
    OLED_SDIN_Set();
}
```

### 7.2 字节写入

I2C 发字节的核心是“把位值放到 SDA，上升沿让从机采样”：

```c
void Write_IIC_Byte(unsigned char IIC_Byte)
{
    unsigned char i;
    unsigned char m, da;
    da = IIC_Byte;
    OLED_SCLK_Clr();
    for (i = 0; i < 8; i++)
    {
        m = da & 0x80;
        if (m == 0x80)
            OLED_SDIN_Set();
        else
            OLED_SDIN_Clr();
        da = da << 1;
        OLED_SCLK_Set();
        OLED_SCLK_Clr();
    }
}
```

这里是高位先发。

### 7.3 ACK 处理被简化了

`IIC_Wait_Ack()` 只有时序，没有真正读取 SDA：

```c
void IIC_Wait_Ack(void)
{
    OLED_SCLK_Set();
    OLED_SCLK_Clr();
}
```

也就是说，这里并没有严格检查从机是否真的应答。

这种写法的好处是简单，坏处是总线异常时不容易尽早发现问题。对 demo 来说够用，但从工程角度看不算完整。

### 7.4 命令和数据是怎么区分的

SSD1306 通过控制字节区分后续内容是命令还是数据。

写命令：

```c
void Write_IIC_Command(unsigned char IIC_Command)
{
    IIC_Start();
    Write_IIC_Byte(0x78);
    IIC_Wait_Ack();
    Write_IIC_Byte(0x00);
    IIC_Wait_Ack();
    Write_IIC_Byte(IIC_Command);
    IIC_Wait_Ack();
    IIC_Stop();
}
```

写数据：

```c
void Write_IIC_Data(unsigned char IIC_Data)
{
    IIC_Start();
    Write_IIC_Byte(0x78);
    IIC_Wait_Ack();
    Write_IIC_Byte(0x40);
    IIC_Wait_Ack();
    Write_IIC_Byte(IIC_Data);
    IIC_Wait_Ack();
    IIC_Stop();
}
```

这里有两个关键值：

- `0x00`：后面跟的是命令
- `0x40`：后面跟的是显示数据

`OLED_WR_Byte()` 只是对这两条路径做了一层包装：

```c
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
    if (cmd)
        Write_IIC_Data(dat);
    else
        Write_IIC_Command(dat);
}
```

## 8. OLED 初始化过程

`OLED_Init()` 先初始化 `PB0/PB1`，再连续发送 SSD1306 的配置命令。

GPIO 初始化部分：

```c
GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOB, &GPIO_InitStructure);
GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);
```

后面的命令序列例如：

```c
OLED_WR_Byte(0xAE, OLED_CMD);
OLED_WR_Byte(0xA1, OLED_CMD);
OLED_WR_Byte(0xC8, OLED_CMD);
OLED_WR_Byte(0x8D, OLED_CMD);
OLED_WR_Byte(0x14, OLED_CMD);
OLED_WR_Byte(0xAF, OLED_CMD);
```

这些命令主要用于：

- 关闭显示并进入初始化状态
- 设置页地址、列地址和扫描方向
- 配置对比度和电荷泵
- 最后打开显示

如果把 SSD1306 看成“显示 RAM + 控制器”，初始化就是在建立这两者之间的映射关系。

## 9. OLED 文本和数字显示

### 9.1 文本显示本质上是写字模

`OLED_ShowChar()` 并不是“调用字体引擎”，而是从字模表里取出对应像素数据，再写到显存里：

```c
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size)
{
    unsigned char c = 0, i = 0;
    c = chr - ' ';
    if (Char_Size == 16)
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
        OLED_Set_Pos(x, y + 1);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
    }
}
```

字模来自 `oledfont.h` 中的 `F8X16` 和 `F6x8`。

### 9.2 为什么 y 坐标是 0、2、4、6

主函数里 4 行文本分别画在：

```c
OLED_ShowString(0, 0, (u8 *)"DHT11 Demo", 16);
OLED_ShowString(0, 2, (u8 *)"Temp:     C", 16);
OLED_ShowString(0, 4, (u8 *)"Humi:     %", 16);
OLED_ShowString(0, 6, (u8 *)"Status: Init", 16);
```

原因是 SSD1306 常用页寻址：

- 1 页对应 8 个垂直像素
- 16 点阵字符会占 2 页

所以 4 行 16 点阵字符刚好对应：

- 第 1 行：页 `0/1`
- 第 2 行：页 `2/3`
- 第 3 行：页 `4/5`
- 第 4 行：页 `6/7`

### 9.3 数字显示

`OLED_ShowNum()` 先把整数拆成十进制各位，再逐位显示：

```c
temp = (num / oled_pow(10, len - t - 1)) % 10;
OLED_ShowChar(x + (size2 / 2) * t, y, temp + '0', size2);
```

这是字符型 OLED 上最常见的数字显示方式。

## 10. 主循环如何组织显示逻辑

主循环的核心语句是：

```c
ret = DHT11_Read_Data(&temp, &humi);
```

读取成功时：

```c
OLED_ShowNum(48, 2, temp, 3, 16);
OLED_ShowNum(48, 4, humi, 3, 16);
OLED_ShowString(0, 6, (u8 *)"Status: OK  ", 16);
```

读取失败时：

```c
OLED_ShowString(48, 2, (u8 *)"---", 16);
OLED_ShowString(48, 4, (u8 *)"---", 16);
if(ret == 1)
    OLED_ShowString(0, 6, (u8 *)"Status: NoAck", 16);
else
    OLED_ShowString(0, 6, (u8 *)"Status: CRC ", 16);
```

这里有一个容易忽略的显示细节：状态字符串采用整行重写，而不是局部覆盖。

例如原来显示的是：

```text
Status: Init
```

如果后面只把状态位置更新成 `OK`，旧字符可能残留在屏幕上。  
因此这里直接写：

```c
OLED_ShowString(0, 6, (u8 *)"Status: OK  ", 16);
```

这样最稳妥。

## 11. 串口输出的作用

主循环每次读取都会输出：

```c
UsartPrintf(USART_DEBUG, "[DHT11] Temp: %d C, Humi: %d %%\r\n", temp, humi);
```

串口的作用主要有两个：

1. 验证 DHT11 协议读出来的数据是否正常
2. 当 OLED 没有显示时，仍然保留调试出口

排查问题时，通常先看串口，再看 OLED。

## 12. 代码里的几个工程细节

### 12.1 读取间隔设为 1000 ms

```c
#define READ_INTERVAL_MS  1000
```

DHT11 不适合高频读取，1 秒一次更稳妥。

### 12.2 `DHT11_Init()` 已经做了存在性检查

```c
uint8_t DHT11_Init(void)
{
    ...
    DHT11_Rst();
    return DHT11_Check();
}
```

也就是说，初始化阶段已经能判断传感器是否在线。  
当前 `demo_main.c` 调用了这个函数，但没有对返回值做分支处理。

### 12.3 OLED 驱动偏 demo 风格

主要体现在两点：

- ACK 没有真正采样
- 写一个字节就起始和停止一次，总线效率不高

这类写法便于理解，但不是追求性能或鲁棒性时的最终形态。

## 13. 可以继续改进的点

### 13.1 补全 ACK 检查

让 `IIC_Wait_Ack()` 真正读取 SDA，总线异常时能更早发现问题。

### 13.2 增加 DHT11 重试

当前读取失败后直接返回。可以在主循环里增加 2 到 3 次快速重试，提高稳定性。

### 13.3 初始化阶段显示硬件状态

例如：

- `Status: Sensor OK`
- `Status: Sensor Miss`
- `Status: OLED OK`

这样上电后更容易判断问题落在哪一层。

### 13.4 抽离引脚配置

当前 DHT11 固定在 `PA0`，OLED 固定在 `PB0/PB1`。  
如果后续准备复用这套代码，最好把引脚定义集中管理。

## 14. 总结

这个 demo 的结构很典型：

- `SysTick` 提供延时基础
- DHT11 负责温湿度数据源
- GPIO 模拟 I2C 驱动 SSD1306
- OLED 负责本地显示
- UART1 负责调试输出

从实现方式上看，它属于标准的“主循环 + 外设驱动”结构。难点不在主循环，而在时序和显示驱动的细节。

用一句话概括，这份代码做的事情就是：

> STM32 按 DHT11 时序读出温湿度，再把结果通过软件 I2C 写到 OLED，同时输出到串口。
