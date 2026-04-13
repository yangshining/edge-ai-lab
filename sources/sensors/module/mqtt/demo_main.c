/**
 * @file    demo_main.c
 * @brief   MQTT 协议封包 demo — 离线测试 CONNECT/PUBLISH 封包和 CONNACK 解包
 * @board   STM32F103C8T6 @ 72 MHz
 * @pins    无硬件引脚依赖（离线测试）
 * @uart    UART1 PA9/PA10 115200 baud (debug output)
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
    unsigned char connack_bytes[] = {0x20, 0x02, 0x00, 0x00};
    unsigned char connack_result;

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
        const char *topic             = "test/topic";
        const char *payload           = "hello";
        const unsigned short payload_len = sizeof("hello") - 1;   /* exclude NUL */
        if(MQTT_PacketPublish(10, topic, payload, payload_len,
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
    if(MQTT_UnPacketRecv(connack_bytes) == MQTT_PKT_CONNACK)
    {
        connack_result = MQTT_UnPacketConnectAck(connack_bytes);
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
