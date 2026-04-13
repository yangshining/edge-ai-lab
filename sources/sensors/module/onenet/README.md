# module/onenet — OneNET 数据上传 Demo

完整的 OneNET 物联网云平台上传链路验证：WiFi 连接 → TCP 建立 → MQTT 鉴权 → 发布传感器数据 → 接收云端下行命令。

---

## A. 模块实现原理

### 1. 整体架构

数据流经六层：

```
传感器数据 → JSON 组包 → MQTT PUBLISH 封包 → ESP8266 AT+CIPSEND → TCP → OneNET云平台
```

各层对应模块：

| 层次 | 功能 | 代码模块 |
|------|------|----------|
| 应用层 | 传感器数据 + 阈值 + 执行器状态 | `demo_main.c` |
| OneNET层 | JSON 组包、DevLink、Subscribe、RevPro | `src/onenet.c` |
| MQTT层 | CONNECT/PUBLISH/SUBSCRIBE 协议封包 | `src/MqttKit.c` |
| 鉴权层 | HMAC-SHA1 签名 + Base64 + URL 编码 | `src/hmac_sha1.c`, `src/base64.c` |
| WiFi/TCP层 | AT 命令驱动，ESP8266 收发 | `src/esp8266.c` |
| 数据解析层 | 云端下行 JSON 解析 | `src/cJSON.c` |

---

### 2. 第一层：HMAC-SHA1 鉴权令牌生成（`OneNET_Authorization`）

OneNET 使用 token 鉴权，token 由以下步骤生成：

1. **Base64 解码** `ACCESS_KEY`（控制台中的 ACCESS_KEY 是 Base64 编码的原始密钥）：
   ```c
   BASE64_Decode(access_key_base64, sizeof(access_key_base64), &olen,
                 (unsigned char *)access_key, strlen(access_key));
   ```

2. **构造 `string_for_signature`**（签名原文）：
   ```
   {et}\n{method}\nproducts/{PROID}/devices/{DEVICE_NAME}\n{ver}
   ```
   例如：`1956499200\nsha1\nproducts/2b785VX4e7/devices/konhqi12121\n2018-10-31`

3. **HMAC-SHA1 签名**：用 Base64 解码后的原始密钥对签名原文做 HMAC-SHA1：
   ```c
   hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
             (unsigned char *)string_for_signature, strlen(string_for_signature),
             (unsigned char *)hmac_sha1_buf);
   ```

4. **Base64 编码**签名摘要，再做 **URL 编码**（`+`→`%2B`，`/`→`%2F` 等）。

5. **拼接 token 字符串**：
   ```
   version=2018-10-31&res=products%2F{PROID}%2Fdevices%2F{DEVICE_NAME}&et={et}&method=sha1&sign={sign}
   ```

---

### 3. 第二层：MQTT CONNECT 握手（`OneNet_DevLink`）

`MQTT_PacketConnect` 参数映射：

| MQTT 字段 | 填入内容 |
|-----------|---------|
| clientId | `DEVICE_NAME`（设备名） |
| username | `PROID`（产品ID） |
| password | token 字符串 |
| keepAlive | 1000 秒 |

`MQTT_UnPacketConnectAck` 返回码含义：

| 返回码 | 含义 |
|--------|------|
| 0 | 连接成功 |
| 1 | 协议版本错误 |
| 2 | 非法 clientId |
| 3 | 服务器内部错误 |
| 4 | 用户名/密码错误（token 格式有误） |
| 5 | 未授权（token 签名不正确或已过期） |

---

### 4. 第三层：订阅下行主题（`OneNET_Subscribe`）

订阅 topic 格式：
```
$sys/{PROID}/{DEVICE_NAME}/thing/property/set
```

OneNET 云平台通过此 topic 下发属性设置命令（JSON），设备订阅后可接收 `fan`、`led`、`temp_adj`、`humi_adj`、`light_th` 等属性的修改指令。

---

### 5. 第四层：JSON 数据组包（`OneNet_FillBuf`）

物模型 JSON 格式（OneNET Studio 物模型规范）：
```json
{
  "id": "123",
  "params": {
    "temp":  {"value": 25},
    "humi":  {"value": 60},
    "light": {"value": 45},
    "mq2":   {"value": 0},
    "mq3":   {"value": 0},
    "mq4":   {"value": 0},
    "mq7":   {"value": 0},
    "beep":  {"value": false}
  }
}
```

- `#ifdef UPLOAD_MQ_DATA` 控制是否上传 MQ2/MQ3/MQ4/MQ7 气体数据（默认开启）。
- `Beep_Status` 在 demo 中由 `stub_actuators.h` 宏替换为常量 `0`，上传结果为 `false`。
- 上传字段 `light`（光照强度，0–99）——历史版本曾误命名为 `smog1`，当前已修正。

---

### 6. 第五层：MQTT PUBLISH 发送（`OneNet_SendData`）

1. `OneNet_FillBuf` 填充 JSON 字符串到 `buf`。
2. `MQTT_PacketSaveData` 封装 MQTT PUBLISH 包，topic 为：
   ```
   $sys/{PROID}/{DEVICE_NAME}/thing/property/post
   ```
3. `ESP8266_SendData` 执行发送：先发 `AT+CIPSEND=N\r\n`，等待 `>` 提示符后发送二进制数据包。

---

### 7. 第六层：云端下行命令处理（`OneNet_RevPro`）

`OneNet_RevPro` 接收 ESP8266 缓冲数据，通过 `MQTT_UnPacketRecv` 判断包类型：

- **MQTT_PKT_PUBLISH**：收到云端属性设置命令，cJSON 解析 `params` 字段：
  - `fan`（bool）→ `Fan_Set(FAN_ON/OFF)` + `fan_mode = 3`
  - `led`（bool）→ `Led_Set(LED_ON/OFF)` + `led_mode = 3`
  - `temp_adj`（int）→ 更新温度报警阈值
  - `humi_adj`（int）→ 更新湿度报警阈值
  - `light_th`（int）→ 更新光照阈值（用于 LED 补光灯自动控制）

- **MQTT_PKT_PUBACK**：PUBLISH 确认（QoS 1 时触发）。
- **MQTT_PKT_SUBACK**：SUBSCRIBE 确认。

**mode=3 覆盖逻辑**：当云端手动控制风扇或 LED 时，`fan_mode` / `led_mode` 被置为 `3`，主循环中 `Update_Actuators()` 检测到 mode=3 时跳过阈值驱动逻辑，保持云端命令的状态。即使云端发 `false` 关闭执行器，mode 仍保持 `3`（不回退到 `1`），防止自动阈值逻辑在下一个 tick 立即重新激活执行器。

---

### 8. 关键设计要点

**Volatile ISR 缓冲区**
`esp8266_buf` 由 `USART2_IRQHandler` 写入（volatile），在 `ESP8266_SendCmd` 中通过 `memcpy` 快照到本地 `snap[]` 后再用 `strstr` 检索，避免编译器优化导致的读取竞争。

**BASE64 解码密钥**
`ACCESS_KEY` 是 Base64 编码的原始密钥（OneNET 控制台展示形式）。HMAC-SHA1 签名必须使用 **解码后的原始字节**，而非 Base64 字符串本身，否则鉴权必然失败（返回码 5）。

**Stub 设计**
`onenet.c` 本身不依赖 actuator 驱动，但通过 `extern` 变量共享 `demo_main.c` 中的全局状态（`fan_mode`、`led_mode`、阈值等）。`stub_actuators.h` 提供 `Fan_Set`/`Led_Set` 的空实现和 `Mq*_GetPercentage()` 的零值内联函数，使模块脱离完整固件也能独立编译，不影响数据上传功能。

---

## B. Demo 使用指南

### 1. 引脚配置

| 信号 | 引脚 | 说明 |
|------|------|------|
| ESP8266 TX | PA3 (USART2 RX) | WiFi 模块发送 |
| ESP8266 RX | PA2 (USART2 TX) | WiFi 模块接收 |
| Debug UART TX | PA9 (USART1 TX) | 串口调试输出 |
| Debug UART RX | PA10 (USART1 RX) | （可选）串口调试输入 |

### 2. 必须修改的配置项

**`src/onenet.c`**（OneNET 平台凭证）：
```c
#define PROID        "your_product_id"    // 产品 ID
#define ACCESS_KEY   "your_access_key"    // 产品级 ACCESS_KEY（Base64 编码）
#define DEVICE_NAME  "your_device_name"   // 设备名称
```

**`src/esp8266.c`**（WiFi 凭证）：
```c
#define ESP8266_WIFI_INFO  "AT+CWJAP=\"your_ssid\",\"your_password\"\r\n"
```

> **Token 有效期**：`OneNET_Authorization` 中 `et=1956499200`，对应北京时间 2032-01-01，有效至约 2032 年，无需修改。

### 3. 编译与烧录

1. 用 **Keil MDK-ARM V5** 打开 `onenet.uvprojx`
2. 按 `F7` 编译，输出文件位于 `output/onenet.hex`
3. 通过 ST-Link 烧录到 STM32F103C8T6
4. 用串口工具（115200 baud）连接 PA9/PA10 观察调试输出

### 4. 预期串口输出

```
[OneNET] Demo start
[OneNET] Demo data: temp=25, humi=60, light=45

[OneNET] Step 1: ESP8266 WiFi init
[ESP8266] Init start
[ESP8266] 1/4 AT OK
[ESP8266] 2/4 CWMODE=1 OK
[ESP8266] 3/4 CWDHCP OK
[ESP8266] 4/4 WiFi connected OK

[OneNET] Step 2: TCP connect to mqtts.heclouds.com:1883
[OneNET] TCP connected

[OneNET] Step 3: MQTT DevLink (HMAC-SHA1 auth)
OneNET_DevLink
NAME: konhqi12121,  PROID: 2b785VX4e7,  KEY:version=2018-10-31&...
Tips:   Connection successful
[OneNET] MQTT connected (CONNACK = 0)

[OneNET] Step 4: Subscribe to property/set
Subscribe Topic: $sys/2b785VX4e7/konhqi12121/thing/property/set

[OneNET] Step 5: Publish demo data
[OneNET] Publish sent

[OneNET] Step 6: Poll cloud response (2s)
[OneNET] No cloud response (normal for QoS 0)

[OneNET] Demo complete
```

### 5. 源码修改说明

本 demo 对两个复制文件做了最小改动：

| 文件 | 改动 | 原因 |
|------|------|------|
| `src/onenet.c` | 将 `#include "beep.h"` / `"fan.h"` / `"mq.h"` 替换为 `#include "stub_actuators.h"` | 消除对完整固件 actuator 驱动的依赖 |
| `src/esp8266.c` | `ESP8266_Init()` 中用 `UsartPrintf` 替换全部 `OLED_Clear()` / `OLED_ShowString()` 调用；开头添加 `Usart2_Init(115200)` | OLED 驱动不在此模块范围内 |
| `core/startup/arm/startup_stm32f10x_md.s` | `Heap_Size EQU 0x00000400` | MqttKit 使用 malloc，需要保留堆空间 |

所有改动处均有 `/* MODULE CHANGE: ... */` 注释标记。

### 6. 复用说明

在其他项目中复用 OneNET 上传能力时，最小文件集：

```
src/onenet.c     — OneNET 业务逻辑（需修改凭证宏）
src/esp8266.c    — WiFi AT 驱动（需修改 ESP8266_WIFI_INFO）
src/MqttKit.c    — MQTT 协议
src/hmac_sha1.c  — HMAC-SHA1
src/base64.c     — Base64
src/cJSON.c      — JSON 解析
inc/onenet.h     — 公开接口
inc/esp8266.h
inc/MqttKit.h
inc/Common.h
inc/hmac_sha1.h
inc/base64.h
inc/cJSON.h
inc/stub_actuators.h  — 若无实际 actuator 驱动则保留
```

**关键调用顺序**：

```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
Delay_Init();
Usart1_Init(115200);          // 调试输出
ESP8266_Init();               // WiFi 连接（内部调用 Usart2_Init）
ESP8266_SendCmd("AT+CIPSTART=...", "CONNECT");  // TCP 连接 OneNET
OneNet_DevLink();             // MQTT 鉴权
OneNET_Subscribe();           // 订阅下行 topic
OneNet_SendData();            // 上传数据
// 循环中：ESP8266_GetIPD → OneNet_RevPro 处理云端命令
```
