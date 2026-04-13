#ifndef _MQTTKIT_H_
#define _MQTTKIT_H_


#include "Common.h"


/* Memory allocation — defaults to C stdlib; replace with RTOS allocator if needed */
#include <stdlib.h>

#define MQTT_MallocBuffer	malloc
#define MQTT_FreeBuffer		free


#define MOSQ_MSB(A)         (uint8)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A)         (uint8)(A & 0x00FF)


/* Memory allocation type flags */
#define MEM_FLAG_NULL		0
#define MEM_FLAG_ALLOC		1
#define MEM_FLAG_STATIC		2


typedef struct Buffer
{

	uint8	*_data;		/* packet data buffer */

	uint32	_len;		/* current write position / filled length */

	uint32	_size;		/* total buffer capacity */

	uint8	_memFlag;	/* memory allocation type: 0=unused, 1=dynamic, 2=static */

} MQTT_PACKET_STRUCTURE;


/* Fixed header packet type enumeration */
enum MqttPacketType
{

    MQTT_PKT_CONNECT = 1, /**< CONNECT packet */
    MQTT_PKT_CONNACK,     /**< CONNACK packet */
    MQTT_PKT_PUBLISH,     /**< PUBLISH packet */
    MQTT_PKT_PUBACK,      /**< PUBACK packet */
    MQTT_PKT_PUBREC,      /**< PUBREC (QoS2 ack1) */
    MQTT_PKT_PUBREL,      /**< PUBREL (QoS2 ack2) */
    MQTT_PKT_PUBCOMP,     /**< PUBCOMP (QoS2 ack3) */
    MQTT_PKT_SUBSCRIBE,   /**< SUBSCRIBE packet */
    MQTT_PKT_SUBACK,      /**< SUBACK packet */
    MQTT_PKT_UNSUBSCRIBE, /**< UNSUBSCRIBE packet */
    MQTT_PKT_UNSUBACK,    /**< UNSUBACK packet */
    MQTT_PKT_PINGREQ,     /**< PINGREQ packet */
    MQTT_PKT_PINGRESP,    /**< PINGRESP packet */
    MQTT_PKT_DISCONNECT,  /**< DISCONNECT packet */

	/* extension */

	MQTT_PKT_CMD  		 /**< Command topic packet (extension) */

};


/* MQTT QoS level */
enum MqttQosLevel
{

    MQTT_QOS_LEVEL0,  /**< at-most-once delivery */
    MQTT_QOS_LEVEL1,  /**< at-least-once delivery */
    MQTT_QOS_LEVEL2   /**< exactly-once delivery */

};


/* MQTT CONNECT flag bits (internal use) */
enum MqttConnectFlag
{

    MQTT_CONNECT_CLEAN_SESSION  = 0x02,
    MQTT_CONNECT_WILL_FLAG      = 0x04,
    MQTT_CONNECT_WILL_QOS0      = 0x00,
    MQTT_CONNECT_WILL_QOS1      = 0x08,
    MQTT_CONNECT_WILL_QOS2      = 0x10,
    MQTT_CONNECT_WILL_RETAIN    = 0x20,
    MQTT_CONNECT_PASSORD        = 0x40,
    MQTT_CONNECT_USER_NAME      = 0x80

};


/* Packet ID constants (user-defined) */
#define MQTT_PUBLISH_ID			10

#define MQTT_SUBSCRIBE_ID		20

#define MQTT_UNSUBSCRIBE_ID		30


/* delete */
void MQTT_DeleteBuffer(MQTT_PACKET_STRUCTURE *mqttPacket);

/* receive */
uint8 MQTT_UnPacketRecv(uint8 *dataPtr);

/* connect packet */
uint8 MQTT_PacketConnect(const int8 *user, const int8 *password, const int8 *devid,
						uint16 cTime, uint1 clean_session, uint1 qos,
						const int8 *will_topic, const int8 *will_msg, int32 will_retain,
						MQTT_PACKET_STRUCTURE *mqttPacket);

/* disconnect packet */
uint1 MQTT_PacketDisConnect(MQTT_PACKET_STRUCTURE *mqttPacket);

/* connect ack */
uint8 MQTT_UnPacketConnectAck(uint8 *rev_data);

/* data upload packet */
uint1 MQTT_PacketSaveData(const int8 *pro_id, const char *dev_name,
								int16 send_len, int8 *type_bin_head, MQTT_PACKET_STRUCTURE *mqttPacket);

/* binary file upload packet */
uint1 MQTT_PacketSaveBinData(const int8 *name, int16 file_len, MQTT_PACKET_STRUCTURE *mqttPacket);

/* command topic parse */
uint8 MQTT_UnPacketCmd(uint8 *rev_data, int8 **cmdid, int8 **req, uint16 *req_len);

/* command response packet */
uint1 MQTT_PacketCmdResp(const int8 *cmdid, const int8 *req, MQTT_PACKET_STRUCTURE *mqttPacket);

/* subscribe packet */
uint8 MQTT_PacketSubscribe(uint16 pkt_id, enum MqttQosLevel qos, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket);

/* subscribe ack */
uint8 MQTT_UnPacketSubscribe(uint8 *rev_data);

/* unsubscribe packet */
uint8 MQTT_PacketUnSubscribe(uint16 pkt_id, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket);

/* unsubscribe ack */
uint1 MQTT_UnPacketUnSubscribe(uint8 *rev_data);

/* publish packet */
uint8 MQTT_PacketPublish(uint16 pkt_id, const int8 *topic,
						const int8 *payload, uint32 payload_len,
						enum MqttQosLevel qos, int32 retain, int32 own,
						MQTT_PACKET_STRUCTURE *mqttPacket);

/* publish parse */
uint8 MQTT_UnPacketPublish(uint8 *rev_data, int8 **topic, uint16 *topic_len, int8 **payload, uint16 *payload_len, uint8 *qos, uint16 *pkt_id);

/* publish ack packet */
uint1 MQTT_PacketPublishAck(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/* publish ack parse */
uint1 MQTT_UnPacketPublishAck(uint8 *rev_data);

/* publish rec packet */
uint1 MQTT_PacketPublishRec(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/* publish rec parse */
uint1 MQTT_UnPacketPublishRec(uint8 *rev_data);

/* publish rel packet */
uint1 MQTT_PacketPublishRel(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/* publish rel parse */
uint1 MQTT_UnPacketPublishRel(uint8 *rev_data, uint16 pkt_id);

/* publish comp packet */
uint1 MQTT_PacketPublishComp(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/* publish comp parse */
uint1 MQTT_UnPacketPublishComp(uint8 *rev_data);

/* ping packet */
uint1 MQTT_PacketPing(MQTT_PACKET_STRUCTURE *mqttPacket);


#endif
