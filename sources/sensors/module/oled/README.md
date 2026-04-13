# module/oled — OLED 显示模块 Demo

## A. 模块实现原理

### 1. 功能说明

本模块驱动一块基于 **SSD1306** 控制器的 0.96 英寸单色 OLED 屏（128×64 像素），
通过软件模拟 I2C（bit-bang）与主控 STM32F103C8T6 通信，无需占用硬件 I2C 外设。
提供字符串显示、数字显示、位图绘制等功能，可用于状态信息、传感器数值的实时展示。

### 2. 整体架构

```
OLED 屏 (SSD1306)
     |
  软件 I2C (bit-bang)
     |
  PB0 = SDA
  PB1 = SCL
     |
STM32F103C8T6
```

驱动层不依赖 STM32 硬件 I2C 外设（`stm32f10x_i2c.c` 虽然编译进来但未使用），
全部时序由 GPIO 直接操作实现，移植方便。

### 3. 核心实现流程

#### 3.1 I2C bit-bang 实现原理

GPIO 引脚在 `OLED_Init()` 中初始化为推挽输出（50 MHz）：

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOB, &GPIO_InitStructure);
```

SCL / SDA 操作通过宏定义完成：

```c
#define OLED_SCLK_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_1)  // SCL = 0
#define OLED_SCLK_Set() GPIO_SetBits(GPIOB,  GPIO_Pin_1)   // SCL = 1
#define OLED_SDIN_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_0)  // SDA = 0
#define OLED_SDIN_Set() GPIO_SetBits(GPIOB,  GPIO_Pin_0)   // SDA = 1
```

标准 I2C 时序：

- **START**：SCL 高期间 SDA 下降沿 → `IIC_Start()`
- **发送字节**：SCL 低时更新 SDA，SCL 产生高脉冲 → `Write_IIC_Byte()`
- **ACK**：拉高 SCL 再拉低（本实现不读取 SDA 返回值）→ `IIC_Wait_Ack()`
- **STOP**：SCL 高期间 SDA 上升沿 → `IIC_Stop()`

字节传输（MSB first）：

```c
void Write_IIC_Byte(unsigned char IIC_Byte)
{
    unsigned char i, m, da = IIC_Byte;
    OLED_SCLK_Clr();
    for (i = 0; i < 8; i++) {
        m = da & 0x80;
        if (m == 0x80) OLED_SDIN_Set(); else OLED_SDIN_Clr();
        da <<= 1;
        OLED_SCLK_Set();
        OLED_SCLK_Clr();
    }
}
```

#### 3.2 OLED 命令 vs 数据字节区分

SSD1306 I2C 地址为 `0x78`（7-bit 地址 `0x3C`，写模式）。
每次传输的第二字节为**控制字节**，决定后续字节的类型：

| 控制字节 | 含义 |
|----------|------|
| `0x00`   | 后续字节为**命令**（Co=0, D/C#=0） |
| `0x40`   | 后续字节为**数据**（Co=0, D/C#=1） |

```c
// 发送命令
void Write_IIC_Command(unsigned char cmd)
{
    IIC_Start();
    Write_IIC_Byte(0x78);   // 从机地址
    IIC_Wait_Ack();
    Write_IIC_Byte(0x00);   // 控制字节：命令模式
    IIC_Wait_Ack();
    Write_IIC_Byte(cmd);    // 命令内容
    IIC_Wait_Ack();
    IIC_Stop();
}

// 发送数据
void Write_IIC_Data(unsigned char dat)
{
    IIC_Start();
    Write_IIC_Byte(0x78);   // 从机地址
    IIC_Wait_Ack();
    Write_IIC_Byte(0x40);   // 控制字节：数据模式
    IIC_Wait_Ack();
    Write_IIC_Byte(dat);    // 像素数据
    IIC_Wait_Ack();
    IIC_Stop();
}
```

上层统一接口 `OLED_WR_Byte(dat, cmd)`：`cmd=0` 发命令，`cmd=1` 发数据。

#### 3.3 OLED 页面地址模式：128 列 × 8 页

SSD1306 GDDRAM 按**页（Page）**组织，共 8 页（Page 0 ~ Page 7），
每页高度 8 个像素，总高度 8 × 8 = 64 像素：

```
Page 0: 像素行 0~7
Page 1: 像素行 8~15
Page 2: 像素行 16~23
...
Page 7: 像素行 56~63
```

定位到某页某列使用 `OLED_Set_Pos(x_col, y_page)`：

```c
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WR_Byte(0xB0 + y, OLED_CMD);              // 设置页地址
    OLED_WR_Byte(((x & 0xF0) >> 4) | 0x10, OLED_CMD); // 列地址高4位
    OLED_WR_Byte((x & 0x0F), OLED_CMD);            // 列地址低4位
}
```

#### 3.4 `OLED_ShowString` 渲染流程

字模存储在 `oledfont.h`，ASCII 偏移量从空格（`0x20`）开始：

- **F6x8**：6×8 点阵，每字符 6 字节，适合 8pt 字体
- **F8X16**：8×16 点阵，每字符 16 字节（上半部分 8 字节 + 下半部分 8 字节），适合 16pt 字体

`OLED_ShowChar` 16pt 渲染（`Char_Size == 16`）：

```c
c = chr - ' ';                          // 字符在字库中的偏移
OLED_Set_Pos(x, y);
for (i = 0; i < 8; i++)
    OLED_WR_Byte(F8X16[c*16 + i], OLED_DATA);      // 上半部分（第 y 页）
OLED_Set_Pos(x, y + 1);
for (i = 0; i < 8; i++)
    OLED_WR_Byte(F8X16[c*16 + i + 8], OLED_DATA);  // 下半部分（第 y+1 页）
```

`OLED_ShowString` 逐字符调用 `OLED_ShowChar`，每字符列偏移 +8，
超过列宽 120 时自动换行（y += 2，对应下移两页即 16 像素）。

#### 3.5 坐标系说明

```c
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 Char_Size);
```

| 参数 | 含义 | 取值范围 |
|------|------|----------|
| `x` | 起始列（像素列号）| 0 ~ 127 |
| `y` | 起始**页号**（非像素行号）| 0 ~ 7（16pt 字体最大 6） |
| `Char_Size` | 字体大小 | `8`（6×8）或 `16`（8×16） |

16pt 字体（`Char_Size=16`）占用两页（16 像素高），因此显示两行文字时 y 需间隔 2：
- 第 0 行：`y=0`（Page 0 + Page 1）
- 第 1 行：`y=2`（Page 2 + Page 3）
- 第 2 行：`y=4`（Page 4 + Page 5）

### 4. 关键设计要点

- **无硬件 I2C**：纯 GPIO bit-bang，无中断，无 DMA，实现简单但速度受限（适合低速 OLED）。
- **ACK 不检测**：`IIC_Wait_Ack()` 只产生 SCL 脉冲，未读取 SDA 应答位；通信异常不会报错。
- **800 ms 上电延时**：`OLED_Init()` 开头调用 `DelayMs(800)` 等待 OLED 上电稳定。
- **字体大小参数**：传入 `8` 使用 F6x8，传入 `16` 使用 F8X16；传入其他值行为等同 `8`。
- **`OLED_Clear()`**：遍历所有 8 页 × 128 列，写入 `0x00` 清屏，每次调用约 8192 次 I2C 传输。

---

## B. Demo 使用指南

### 1. 引脚配置

| 信号 | 引脚 | 说明 |
|------|------|------|
| OLED SCL | PB1 | 软件 I2C 时钟，推挽输出 |
| OLED SDA | PB0 | 软件 I2C 数据，推挽输出 |
| UART1 TX | PA9 | 调试串口输出，115200 baud |
| UART1 RX | PA10 | （接收，Demo 中未使用） |
| VCC | 3.3V 或 5V | OLED 供电 |
| GND | GND | 共地 |

### 2. Demo 行为

**OLED 屏幕内容：**

```
+--------------------+
| STM32 OLED Demo    |  <- 第 0 行 (y=0, 16pt)
| Hello World        |  <- 第 1 行 (y=2, 16pt)
| Count:    0        |  <- 第 2 行 (y=4, 16pt)，计数每 500 ms 自增
+--------------------+
```

- 上电后静态标题固定显示；
- 第三行计数从 0 开始，每 500 ms 加 1，达到 9999 后归零循环；
- 计数列右对齐（4 位，空位补空格）。

**UART1 输出（115200 baud）：**

```
[OLED] Demo start
[OLED] Static content displayed
[OLED] Count: 0
[OLED] Count: 1
[OLED] Count: 2
...
```

### 3. 编译与烧录

1. 打开 `module/oled/oled.uvprojx`（Keil MDK-ARM V5）
2. 按 **F7** 编译，输出文件位于 `module/oled/output/oled.hex`
3. 通过 ST-Link 连接目标板，下载 `.hex` 到 Flash
4. 复位运行，串口工具以 115200 baud 连接 PA9 观察调试输出

### 4. 可配置项

位于 `demo_main.c` 顶部：

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `REFRESH_INTERVAL_MS` | `500` | 计数刷新间隔（毫秒） |
| `COUNT_MAX` | `9999` | 计数上限，达到后归零 |

### 5. 复用说明

将 OLED 模块集成到其他项目时，只需：

1. 复制 `src/oled.c`、`inc/oled.h`、`inc/oledfont.h` 到目标工程
2. 在 Keil 包含路径中添加 `inc/` 目录
3. 确保已包含 `delay.c` / `delay.h`（`oled.c` 调用 `DelayMs()`）
4. 调用顺序：`OLED_Init()` → `OLED_Clear()` → `OLED_ShowString(...)`

字库仅含 ASCII 可打印字符（0x20 ~ 0x7E）；中文字符需另行添加 `Hzk[]` 字模数组
并调用 `OLED_ShowCHinese()`。
