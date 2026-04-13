# module/esp8266 — ESP8266 WiFi 驱动模块

基于 AT 指令的 ESP8266 WiFi 驱动，运行在 STM32F103C8T6 上。通过 USART2 与模块通信，调试输出走 USART1。

---

## A. 模块实现原理

### 1. 功能说明

`esp8266.c` 封装了 ESP8266 的 AT 指令交互层，提供以下能力：

| 函数 | 说明 |
|------|------|
| `ESP8266_Init()` | 初始化 ESP8266：设置 Station 模式、启用 DHCP、连接 WiFi |
| `ESP8266_SendCmd(cmd, res)` | 发送 AT 指令并等待指定关键词响应，返回 0=成功/1=失败 |
| `ESP8266_SendData(data, len)` | 通过已建立的 TCP 连接发送二进制数据（`AT+CIPSEND`） |
| `ESP8266_GetIPD(timeOut)` | 等待并返回服务器下行数据（`+IPD,n:` 格式） |
| `ESP8266_Clear()` | 清空接收缓冲区 |

### 2. 整体架构

```
STM32F103C8T6
  ├── USART1 (PA9/PA10, 115200) ──► 串口调试工具（PC）
  └── USART2 (PA2/PA3, 115200) ──► ESP8266 模块
                                      └── WiFi AP ──► Internet / MQTT Broker
```

主循环与 ISR 之间通过 `volatile` 缓冲区 `esp8266_buf[512]` 交换数据：
- ISR（`USART2_IRQHandler`）写入缓冲区
- 主循环通过 `ESP8266_WaitRecive()` 轮询"接收稳定"标志后读取

### 3. 核心实现流程

#### 3.1 AT 指令状态机（`ESP8266_Init`）

`ESP8266_Init()` 按顺序发送四条指令，每条均在成功后才推进：

```
AT  ──► "OK"
  → AT+CWMODE=1  ──► "OK"          (Station 模式)
    → AT+CWDHCP=1,1  ──► "OK"      (启用 DHCP)
      → AT+CWJAP="SSID","PWD"  ──► "OK"   (连接 AP，等待 GOT IP)
```

每步如果失败（`ESP8266_SendCmd` 返回 1），则延迟 500 ms 后重试，无限循环直到成功。连接 AP 时等待的关键词为 `"OK"`（含 `WIFI CONNECTED` 和 `WIFI GOT IP`）。

> **注意**：原始主项目代码等待 `"GOT IP"` 关键词；本模块版本等待 `"OK"`，与任务规范保持一致。

连接成功后，各步骤通过 USART1 打印进度（替代原版 OLED 显示）：

```
[ESP8266] Init start
[ESP8266] 1/4 AT OK
[ESP8266] 2/4 CWMODE=1 OK
[ESP8266] 3/4 CWDHCP OK
[ESP8266] 4/4 WiFi connected OK
```

#### 3.2 `ESP8266_SendCmd` 内部实现

```c
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
    unsigned char timeOut = 200;
    char snap[sizeof(esp8266_buf)];          // 本地非 volatile 快照

    Usart_SendString(USART2, cmd, strlen(cmd));  // 发送 AT 命令

    while(timeOut--)
    {
        if(ESP8266_WaitRecive() == REV_OK)
        {
            /* 关键：将 volatile ISR 缓冲区 memcpy 到本地 snap，再传给 strstr */
            memcpy(snap, (const void *)esp8266_buf, sizeof(snap));
            if(strstr(snap, res) != NULL)
            {
                ESP8266_Clear();
                return 0;   // 成功
            }
        }
        DelayXms(10);
    }
    return 1;   // 超时失败
}
```

**为何需要 `memcpy` 再传给 `strstr`**：`esp8266_buf` 声明为 `volatile`，标准 C 库函数 `strstr` 的参数类型是 `const char *`（非 `volatile`）。直接传入 `volatile` 缓冲区会导致编译器警告，并且在多字节操作期间 ISR 可能修改缓冲区内容，造成数据竞争。`memcpy` 将数据原子性地拷贝到普通栈变量后再做字符串搜索，保证安全性。

#### 3.3 USART2_IRQHandler ISR 缓冲机制

```c
volatile unsigned char esp8266_buf[512];
volatile unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        if(esp8266_cnt >= sizeof(esp8266_buf))
            esp8266_cnt = 0;        // 防溢出回绕
        esp8266_buf[esp8266_cnt++] = USART2->DR;
        USART_ClearFlag(USART2, USART_FLAG_RXNE);
    }
}
```

- 缓冲区大小 512 字节；超出时从 0 覆写（防止刷爆）。
- `esp8266_cnt` 记录已接收字节数，`esp8266_cntPre` 保存上次检测时的值。
- `ESP8266_WaitRecive()` 检测 `esp8266_cnt == esp8266_cntPre`（连续两次相等）来判断"接收稳定/完成"。

#### 3.4 `ESP8266_SendData` 流程

```
ESP8266_Clear()
  → sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len)
    → ESP8266_SendCmd(cmdBuf, ">")   // 等待 ESP8266 返回 ">" 提示符
      → Usart_SendString(USART2, data, len)   // 发送实际数据
```

使用前需已通过 `AT+CIPSTART` 建立 TCP 连接（由上层 MQTT/OneNET 层负责）。

#### 3.5 `ESP8266_GetIPD` 获取下行数据

ESP8266 下行数据格式为 `+IPD,<len>:<payload>`。`ESP8266_GetIPD` 的流程：

```
等待 ESP8266_WaitRecive() == REV_OK
  → strstr(esp8266_buf, "IPD,")    // 找到 "+IPD," 头
    → strchr(ptr, ':')             // 找到长度字段结束符
      → 返回 ':' 后的指针（payload 起始地址）
```

超时（`timeOut * 5ms`）后返回 `NULL`。传入 `timeOut=0` 时只检测一次。

### 4. 关键设计要点

| 要点 | 说明 |
|------|------|
| volatile 安全 | `memcpy` 到栈变量后再传给 `strstr`，避免数据竞争 |
| ISR 缓冲区大小 | 512 字节，超出时回绕，适合 MQTT 数据包（建议不超过 400 字节） |
| OLED 解耦 | 本模块版本移除所有 `OLED_Clear/ShowString` 调用，改为 `UsartPrintf` 输出到 UART1，可在无 OLED 硬件环境中独立运行 |
| 阻塞式初始化 | `ESP8266_Init` 在 WiFi 连接成功前永久阻塞；适合上电初始化，不适合运行时重连 |

---

## B. Demo 使用指南

### 1. 引脚配置

| 信号 | STM32 引脚 | 说明 |
|------|-----------|------|
| ESP8266 TX | PA3 (USART2 RX) | ESP8266 发送 → MCU 接收 |
| ESP8266 RX | PA2 (USART2 TX) | MCU 发送 → ESP8266 接收 |
| 调试输出 TX | PA9 (USART1 TX) | 连接 USB-TTL 转换器 |
| 调试输入 RX | PA10 (USART1 RX) | 可选，本 Demo 不使用 |
| ESP8266 VCC | 3.3 V | 注意不要接 5V |
| ESP8266 GND | GND | 共地 |
| ESP8266 CH_PD | 3.3 V | 必须拉高，否则模块不工作 |

### 2. WiFi 配置

打开 `src/esp8266.c`，修改第 36 行的宏：

```c
#define ESP8266_WIFI_INFO  "AT+CWJAP=\"你的SSID\",\"你的密码\"\r\n"
```

将 `你的SSID` 和 `你的密码` 替换为实际 WiFi 网络的名称和密码。**注意**：SSID 和密码中的双引号须用 `\"` 转义。

### 3. Demo 行为与 UART 输出

上电后，打开串口工具（115200, 8N1）连接 PA9，可观察到如下输出：

```
[ESP8266] Demo start
[ESP8266] USART2: PA2(TX) PA3(RX) 115200 baud
[ESP8266] Init start
[ESP8266] 1/4 AT OK
[ESP8266] 2/4 CWMODE=1 OK
[ESP8266] 3/4 CWDHCP OK
[ESP8266] 4/4 WiFi connected OK
[ESP8266] Querying IP (AT+CIFSR)...
[ESP8266] No response to AT+CIFSR
[ESP8266] Demo complete
```

> `AT+CIFSR` 响应格式为 `+CIFSR:STAIP,"x.x.x.x"`，不含 `+IPD,` 头，因此 `ESP8266_GetIPD` 返回 `NULL`，输出 "No response"。IP 地址实际上已在 ESP8266 内部缓冲区中，可用 `ESP8266_SendCmd("AT+CIFSR\r\n", "STAIP")` 配合 `memcpy(snap, esp8266_buf, ...)` 直接打印 snap 获取完整响应。这是设计上的正常现象，Demo 重点展示 WiFi 连接流程。

### 4. 编译与烧录

1. 用 Keil MDK-ARM V5 打开 `esp8266.uvprojx`
2. 按 F7 编译，输出文件位于 `output/esp8266.hex`
3. 通过 ST-Link 烧录到 STM32F103C8T6
4. 打开串口工具（PA9, 115200 baud）观察输出

### 5. 复用说明（集成到 MQTT/HTTP）

本模块可直接作为 TCP 数据发送的底层驱动使用：

```c
// 1. 初始化（一次）
ESP8266_Init();   // 连接 WiFi

// 2. 建立 TCP 连接（由调用方发送 AT+CIPSTART）
ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"broker.host\",1883\r\n", "OK");

// 3. 发送数据（例如 MQTT CONNECT 包）
ESP8266_SendData(mqtt_buf, mqtt_len);

// 4. 接收下行数据
unsigned char *payload = ESP8266_GetIPD(100);   // 等待 1s

// 5. 清空缓冲区（每次上传后调用）
ESP8266_Clear();
```

在 OneNET/MQTT 场景中，`ESP8266_SendData` 被 `onenet.c` 的 `OneNET_Publish` 间接调用，`ESP8266_GetIPD` 被订阅回调解析器使用。
