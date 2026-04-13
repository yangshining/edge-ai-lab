# module/mqtt — MQTT 协议封包 Demo

## A. 模块实现原理

### 1. 整体架构

在完整的 IoT 系统中，MQTT 协议栈位于应用层与传输层之间：

```
应用层 (onenet.c)
    ↓  调用 MQTT_PacketConnect / MQTT_PacketPublish
MQTT 编解码层 (MqttKit.c)  ← 本 demo 验证这一层
    ↓  输出字节流
传输层 (ESP8266 TCP 连接)
```

`MqttKit.c` 只负责协议编解码，不依赖网络硬件，可以完全离线独立测试。本 demo 正是利用这一特性，在无 WiFi 模块的情况下验证 MQTT 协议层的正确性。

---

### 2. 核心实现流程

#### a. CONNECT 包结构（MQTT v3.1.1）

```
Fixed Header:
  [0x10]          — 报文类型 CONNECT (type=1, flags=0)
  [剩余长度]       — 可变长编码（1~4 字节，每字节低7位有效，最高位为延续标志）

Variable Header:
  [0x00 0x04]     — 协议名长度 = 4
  [0x4D 0x51 0x54 0x54]  — "MQTT"
  [0x04]          — 协议级别（MQTT 3.1.1 = 4）
  [Connect Flags] — 例：CleanSession=1 → 0x02；有用户名+密码 → 0xC2
  [MSB  LSB]      — KeepAlive 秒数（大端，2 字节）

Payload:
  [MSB  LSB]  [ClientID 字符串]   — 2 字节长度前缀 + 字符串
  [MSB  LSB]  [Username 字符串]   — 若 Username 标志置位
  [MSB  LSB]  [Password 字符串]   — 若 Password 标志置位
```

代码调用示例：
```c
MQTT_PACKET_STRUCTURE pkt = {NULL, 0, 0, 0};
MQTT_PacketConnect("product_id", "auth_token", "device_id",
                   60, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &pkt);
// pkt._data 指向编码好的字节流，pkt._len 为有效长度
MQTT_DeleteBuffer(&pkt);
```

#### b. PUBLISH 包结构

```
Fixed Header:
  [0x30 | (QoS<<1) | retain]  — 类型=PUBLISH(3)，flags 含 QoS/retain
  [剩余长度]                   — 可变长编码

Variable Header:
  [MSB  LSB]  [topic 字符串]  — topic 长度（大端）+ topic 内容
  [MSB  LSB]                  — Packet Identifier（仅 QoS 1/2 时存在）

Payload:
  [原始消息内容]               — 任意字节，长度 = 剩余长度 - topic区域长度
```

代码调用示例：
```c
MQTT_PacketPublish(10, "test/topic", "hello", 5,
                   MQTT_QOS_LEVEL0, 0, 1, &pkt);
// pkt._data[0] = 0x30（QoS 0，无 retain）
MQTT_DeleteBuffer(&pkt);
```

#### c. CONNACK 包

固定格式，共 4 字节：

| 字节 | 含义 | 值 |
|------|------|----|
| [0]  | 报文类型 | `0x20` |
| [1]  | 剩余长度 | `0x02` |
| [2]  | Session Present 标志 | `0x00` 或 `0x01` |
| [3]  | 连接返回码 | `0`=接受，`1`=协议版本错，`2`=标识符被拒，`3`=服务器不可用，`4`=用户名/密码错，`5`=未授权 |

`MQTT_UnPacketConnectAck` 返回字节 `[3]`，即连接返回码。

#### d. HMAC-SHA1 鉴权令牌（Authentication Token）

在 OneNET 平台上，MQTT CONNECT 包的 `password` 字段并非明文密码，而是一个经过签名的鉴权令牌。令牌的生成步骤为：Base64 解码 `ACCESS_KEY` → 用所得原始密钥对"签名字符串"（`string_for_signature`）做 HMAC-SHA1 → 将结果 Base64 编码 → URL 编码 → 按以下格式拼装最终令牌：

```
version=2018-10-31&res=products%2F{PROID}%2Fdevices%2F{DEVICE_NAME}&et={timestamp}&method=sha1&sign={encoded_sign}
```

| 字段 | 说明 |
|------|------|
| `res` | 资源路径，格式 `products/{PROID}/devices/{DEVICE_NAME}`，URL 编码后斜杠变 `%2F` |
| `et` | 过期时间戳（Unix 秒），超过后令牌失效 |
| `method` | 固定为 `sha1` |
| `sign` | HMAC-SHA1 结果经 Base64 再 URL 编码后的字符串 |

此逻辑由 `NET/onenet/src/onenet.c` 的 `OneNET_Authorization()` 函数实现；`MqttKit.c` 通过 `MQTT_PacketConnect` 的 `password` 参数接收该令牌并将其写入 CONNECT 包的 Payload。本离线 demo 中以 `"demo_token"` 作为占位符传入，不执行实际签名计算。

#### e. 内存管理

`MqttKit.h` 中的映射关系：
```c
#define MQTT_MallocBuffer  malloc
#define MQTT_FreeBuffer    free
```

- **必须**在 Keil 项目中设置 Heap Size（见使用指南第2节），否则 `malloc` 失败，`MQTT_NewBuffer` 返回 `_data == NULL`，后续 `MQTT_PacketConnect` 等函数返回非零错误码，demo 会打印 `FAIL`。
- `MQTT_DeleteBuffer`：释放 `_data` 所指内存（若为动态分配），并将 `_data` 置 `NULL`、`_len/_size/_memFlag` 清零。
- `MQTT_FreeBuffer`：仅是 `free` 的别名，不清零结构体字段。
- **必须**在每次成功编包后调用 `MQTT_DeleteBuffer`，否则内存泄漏；下次调用前 `pkt._data` 须为 `NULL`（零初始化保证）。

#### f. QoS 说明

| QoS 级别 | 行为 | 是否需要 PUBACK |
|----------|------|----------------|
| QoS 0    | 最多发一次，无确认 | 否 |
| QoS 1    | 至少发一次，需等 PUBACK（packet_id 匹配） | 是 |
| QoS 2    | 恰好一次，四次握手（PUBREC→PUBREL→PUBCOMP） | 否（流程更复杂）|

本 demo 使用 QoS 0，PUBLISH 包中无 Packet Identifier 字段。

#### g. MQTT_PACKET_STRUCTURE 字段含义

```c
typedef struct Buffer {
    uint8  *_data;     // 编码后字节流的起始指针（malloc 分配或外部缓冲区）
    uint32  _len;      // 已写入的有效字节数
    uint32  _size;     // 已分配的缓冲区总大小
    uint8   _memFlag;  // 0=未分配  1=动态分配(malloc)  2=静态缓冲区
} MQTT_PACKET_STRUCTURE;
```

初始化时务必将 `_data` 置 `NULL`（`{NULL, 0, 0, 0}`），`MQTT_NewBuffer` 以此判断是否需要 `malloc`。

---

### 3. 关键设计要点

- 堆大小不足时：`MQTT_NewBuffer` → `malloc` 返回 `NULL` → `pkt._data == NULL` → `MQTT_PacketConnect` 返回 `4` → demo 打印 `FAIL`。
- 本 demo **完全离线**，无需 ESP8266 或任何 TCP 连接，可在没有硬件的情况下在仿真器或实际板卡上验证 MQTT 协议层。
- `MQTT_UnPacketRecv` 仅读取 `dataPtr[0]` 高4位判断报文类型，不依赖网络接收，直接传入硬编码字节数组即可。

---

## B. Demo 使用指南

### 1. 引脚配置

| 信号 | 引脚 | 说明 |
|------|------|------|
| UART1 TX | PA9 | 115200 baud，调试输出 |
| UART1 RX | PA10 | 未使用（可不接） |

本 demo 无需其他外设。

---

### 2. 配置项

本 demo **无需修改任何配置**：没有 WiFi SSID、没有云平台凭证、没有运行时参数。它是一个完全离线的协议层测试，上电即可运行。

唯一可选的调整是修改 `demo_main.c` 中的以下三个字符串，以观察不同参数对封包字节流的影响：

| 参数值 | 当前值 | 说明 |
|--------|--------|------|
| `"demo_product"` | `"demo_product"` | CONNECT 包中的 Username 字段 |
| `"demo_token"` | `"demo_token"` | CONNECT 包中的 Password 字段（真实场景为 HMAC-SHA1 令牌） |
| `"demo_client"` | `"demo_client"` | CONNECT 包中的 Client Identifier 字段 |

---

### 3. Keil Heap 设置方法

1. 打开 `mqtt.uvprojx`
2. 菜单 → **Project** → **Options for Target 'mqtt'**
3. 切换到 **Target** 标签页
4. 在 **Code Generation** 区域，将 **Heap Size** 设为 `0x400`（或更大）
5. 点击 OK，重新编译

> 注意：Heap 大小也可直接修改 `core/startup/arm/startup_stm32f10x_md.s` 中的 `Heap_Size EQU 0x00000400`。当前 startup 文件中已设置为 `0x600`，满足要求，无需修改。

---

### 4. 编译与烧录

1. 用 Keil MDK-ARM V5 打开 `module/mqtt/mqtt.uvprojx`
2. 按 **F7** 编译（目标输出到 `output/mqtt.hex`）
3. 通过 ST-Link 将 `mqtt.hex` 烧录到 STM32F103C8T6
4. 用串口工具（115200，8N1）连接 PA9，复位后查看输出

---

### 5. 预期 UART 输出

```
[MQTT] Demo start (offline packet encode/decode test)

[MQTT] Test 1: CONNECT packet
  CONNECT (51 bytes): 10 31 00 04 4D 51 54 54 04 C2 00 3C 00 0B 64 65 6D 6F 5F 63 6C 69 65 6E 74 00 0C 64 65 6D 6F 5F 70 72 6F 64 75 63 74 00 0A 64 65 6D 6F 5F 74 6F 6B 65 6E
  PASS: CONNECT encoded (51 bytes)

[MQTT] Test 2: PUBLISH packet (QoS 0)
  PUBLISH (19 bytes): 30 11 00 0A 74 65 73 74 2F 74 6F 70 69 63 68 65 6C 6C 6F
  PASS: PUBLISH encoded (19 bytes)

[MQTT] Test 3: CONNACK decode
  CONNACK bytes (4 bytes): 20 02 00 00
  CONNACK return code: 0 (ACCEPTED)

[MQTT] Demo complete
```

- `CONNECT` 包固定头 `10 31`：类型=CONNECT，剩余长度=49；`00 04 4D 51 54 54` 为协议名长度+`"MQTT"`；`04` 为协议级别；`C2` 为连接标志（CleanSession=1，Username=1，Password=1）；`00 3C` 为 KeepAlive=60；后续为 ClientID/Username/Password 的 2 字节长度前缀加字符串内容
- `PUBLISH` 包 `30 11 00 0A 74 65 73 74 2F 74 6F 70 69 63 68 65 6C 6C 6F`：`30`=PUBLISH QoS 0，`11`=剩余长度 17，`00 0A`=topic 长度 10，`74 65 73 74 2F 74 6F 70 69 63`="test/topic"，`68 65 6C 6C 6F`="hello"
- CONNACK 返回码 `0` 表示连接被接受

---

### 6. 源码修改说明

`src/MqttKit.c`、`inc/MqttKit.h`、`inc/Common.h` 三个文件均从主工程的 `NET/MQTT/` 目录**原样复制**，未做任何修改。

本模块无需进行 OLED 移除或桩函数注入（与 esp8266、onenet 模块不同，MqttKit 本身不依赖 OLED 或任何外设驱动）。所有测试数据均为硬编码，不依赖外部输入，无需修改任何配置即可运行。

---

### 7. 复用说明

将以下文件复制到目标工程中即可复用 MQTT 编解码功能：

```
src/MqttKit.c   → 放入工程 src 目录
inc/MqttKit.h   → 放入工程 inc 目录
inc/Common.h    → 放入工程 inc 目录（类型定义）
```

典型调用顺序（与 ESP8266 配合）：

```c
// 1. 编码 CONNECT 包
MQTT_PacketConnect(user, password, devid, 60, 1, MQTT_QOS_LEVEL0,
                   NULL, NULL, 0, &pkt);

// 2. 通过 ESP8266 TCP 连接发送
ESP8266_SendData(pkt._data, pkt._len);

// 3. 释放内存
MQTT_DeleteBuffer(&pkt);

// 4. 等待 CONNACK，解析返回码
if (MQTT_UnPacketRecv(recv_buf) == MQTT_PKT_CONNACK) {
    uint8 rc = MQTT_UnPacketConnectAck(recv_buf);
    // rc == 0 表示连接成功
}
```
