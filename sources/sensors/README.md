# STM32F103 智能传感器监控系统

基于 STM32F103C8T6 的 IoT 环境监控固件，通过 ESP8266 WiFi 模块将传感器数据上传至 OneNET 云平台，并根据阈值自动控制执行器。

## 硬件配置

### 传感器

| 传感器 | 引脚 | 功能 |
|--------|------|------|
| DHT11 | PA11 | 温湿度（单总线） |
| MQ2 | PA5 (ADC_CH5) | 烟雾 / LPG / CO |
| MQ3 | PA0 (ADC_CH0) | 酒精 |
| MQ4 | PA1 (ADC_CH1) | 甲烷 / CNG |
| MQ7 | PA6 (ADC_CH6) | 一氧化碳 |
| 光照传感器 | PA4 (ADC_CH4) | 光照强度 |

> **注意**：PA2/PA3 被 USART2（ESP8266）占用，MQ5/MQ6 在本硬件配置下不可用。

### 执行器

| 设备 | 引脚 | 触发条件 |
|------|------|----------|
| 风扇 | PA8 | 温度 ≥ 37°C 或湿度 ≥ 90% |
| 补光 LED | PC14 | 光照 ≤ 30% |
| 蜂鸣器 | PC13 | 任一阈值超标 |

### 通信接口

| 设备 | 接口 | 引脚 |
|------|------|------|
| ESP8266 | USART2 (115200) | TX=PA2, RX=PA3 |
| OLED 128×64 | I2C | SCL=PB1, SDA=PB0 |
| 调试串口 | USART1 (115200) | TX=PA9, RX=PA10 |

## 快速开始

### 1. 开发环境

- Keil MDK-ARM V5
- ARM Compiler 5.06
- ST-Link / J-Link 调试器

### 2. 配置 WiFi 和 OneNET

**WiFi 凭据** — `NET/device/src/esp8266.c`：
```c
#define ESP8266_WIFI_INFO  "AT+CWJAP=\"your_ssid\",\"your_password\"\r\n"
```

**OneNET 设备信息** — `NET/onenet/src/onenet.c`：
```c
#define PROID        "your_product_id"
#define ACCESS_KEY   "your_access_key"
#define DEVICE_NAME  "your_device_name"
```

### 3. 编译与下载

1. 用 Keil 打开 `stm32f103.uvprojx`
2. F7 编译，生成 `output/stm32f103.hex`
3. ST-Link 烧录到 STM32F103C8T6

### 4. 运行

上电后 OLED 依次显示：
```
Hardware init OK  →  ESP8266 Init OK  →  Connect MQTTs...  →  Device login ...
```
登录成功后进入主循环，每 5 秒上传一次传感器数据。调试信息通过 USART1 以 115200 波特率输出。

## 云端数据点

上传到 OneNET 的 JSON 格式（`$sys/{PROID}/{DEVICE_NAME}/thing/property/post`）：

```json
{
  "id": "123",
  "params": {
    "temp":  {"value": 25},
    "humi":  {"value": 60},
    "light": {"value": 72},
    "beep":  {"value": false}
  }
}
```

云端可下发以下属性控制设备：

| 属性 | 类型 | 说明 |
|------|------|------|
| `fan` | bool | 手动控制风扇 |
| `led` | bool | 手动控制补光灯 |
| `temp_adj` | int | 温度报警阈值 |
| `humi_adj` | int | 湿度报警阈值 |
| `light_th` | int | 光照阈值 |

## 调整阈值

默认阈值在 `user/main.c` 开头定义，也可通过 OneNET 云端动态修改：

```c
u8  temp_adj = 37;   // 温度报警阈值 (°C)
u8  humi_adj = 90;   // 湿度报警阈值 (%)
int smog_th  = 50;   // MQ2 烟雾报警阈值 (%)
int light_th = 30;   // 光照补光阈值 (%)
```

## 软件架构

```
user/main.c
  ├── Refresh_Sensors()    读取 DHT11、ADC（光照/MQ2）
  ├── Update_Actuators()   阈值判断 → 控制风扇/LED/蜂鸣器
  └── Refresh_Display()    刷新 OLED

hardware/
  ├── adc.c    DMA 循环采集 5 通道（ADC_IDX_* 宏定义索引）
  ├── mq.c     MQ 传感器读值/电压/百分比
  ├── dht11.c  单总线时序
  ├── fan.c / beep.c    执行器控制

NET/
  ├── device/esp8266.c   AT 指令驱动（USART2）
  ├── MQTT/MqttKit.c     MQTT 协议打包/解包
  └── onenet/onenet.c    OneNET 鉴权、数据上传、命令接收
```

## 常见问题

**ESP8266 连接失败**：检查 WiFi 凭据；确认 ESP8266 供电正常（3.3V，峰值电流 >300mA）；串口查看 AT 响应。

**OneNET 鉴权失败（error 5）**：检查 PROID / ACCESS_KEY / DEVICE_NAME 是否与平台一致；token 过期时间硬编码为 2032 年，暂无需处理。

**传感器数据全为 0**：确认 ADC 引脚无短路；USART2 初始化必须在 ADCx_Init 之前完成（代码中已保证顺序）。

**OLED 无显示**：检查 I2C 拉高电阻（4.7kΩ）；确认 SDA=PB0、SCL=PB1 接线正确。
