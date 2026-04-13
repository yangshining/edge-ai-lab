/** @file MqttKit.c
 * @brief MQTT protocol packet builder/parser (v3.1.1)
 * @version V1.6
 */

#include "MqttKit.h"

#include <string.h>
#include <stdio.h>


#define CMD_TOPIC_PREFIX		"$creq"


/* MQTT_NewBuffer: allocate/initialize packet buffer */
void MQTT_NewBuffer(MQTT_PACKET_STRUCTURE *mqttPacket, uint32 size)
{

	uint32 i = 0;

	if(mqttPacket->_data == NULL)
	{
		mqttPacket->_memFlag = MEM_FLAG_ALLOC;

		mqttPacket->_data = (uint8 *)MQTT_MallocBuffer(size);
		if(mqttPacket->_data != NULL)
		{
			mqttPacket->_len = 0;

			mqttPacket->_size = size;

			for(; i < mqttPacket->_size; i++)
				mqttPacket->_data[i] = 0;
		}
	}
	else
	{
		mqttPacket->_memFlag = MEM_FLAG_STATIC;

		for(; i < mqttPacket->_size; i++)
			mqttPacket->_data[i] = 0;

		mqttPacket->_len = 0;

		if(mqttPacket->_size < size)
			mqttPacket->_data = NULL;
	}

}

/* MQTT_DeleteBuffer: free packet buffer */
void MQTT_DeleteBuffer(MQTT_PACKET_STRUCTURE *mqttPacket)
{

	if(mqttPacket->_memFlag == MEM_FLAG_ALLOC)
		MQTT_FreeBuffer(mqttPacket->_data);

	mqttPacket->_data = NULL;
	mqttPacket->_len = 0;
	mqttPacket->_size = 0;
	mqttPacket->_memFlag = MEM_FLAG_NULL;

}

/* MQTT_DumpLength: encode remaining-length field into bytes */
int32 MQTT_DumpLength(size_t len, uint8 *buf)
{

	int32 i = 0;

	for(i = 1; i <= 4; ++i)
	{
		*buf = len % 128;
		len >>= 7;
		if(len > 0)
		{
			*buf |= 128;
			++buf;
		}
		else
		{
			return i;
		}
	}

	return -1;
}

/* MQTT_ReadLength: decode remaining-length bytes to integer */
int32 MQTT_ReadLength(const uint8 *stream, int32 size, uint32 *len)
{

	int32 i;
	const uint8 *in = stream;
	uint32 multiplier = 1;

	*len = 0;
	for(i = 0; i < size; ++i)
	{
		*len += (in[i] & 0x7f) * multiplier;

		if(!(in[i] & 0x80))
		{
			return i + 1;
		}

		multiplier <<= 7;
		if(multiplier >= 2097152)		//128 * *128 * *128
		{
			return -2;					// error, out of range
		}
	}

	return -1;							// not complete

}

/* MQTT_UnPacketRecv: determine packet type from received data */
uint8 MQTT_UnPacketRecv(uint8 *dataPtr)
{

	uint8 status = 255;
	uint8 type = dataPtr[0] >> 4;

	if(type < 1 || type > 14)
		return status;

	if(type == MQTT_PKT_PUBLISH)
	{
		uint8 *msgPtr;
		uint32 remain_len = 0;

		msgPtr = dataPtr + MQTT_ReadLength(dataPtr + 1, 4, &remain_len) + 1;

		if(remain_len < 2 || dataPtr[0] & 0x01)					//retain
			return 255;

		if(remain_len < ((uint16)msgPtr[0] << 8 | msgPtr[1]) + 2)
			return 255;

		if(strstr((int8 *)msgPtr + 2, CMD_TOPIC_PREFIX) != NULL)	// command topic prefix match
			status = MQTT_PKT_CMD;
		else
			status = MQTT_PKT_PUBLISH;
	}
	else
		status = type;

	return status;

}

/* MQTT_PacketConnect: build CONNECT packet */
uint8 MQTT_PacketConnect(const int8 *user, const int8 *password, const int8 *devid,
						uint16 cTime, uint1 clean_session, uint1 qos,
						const int8 *will_topic, const int8 *will_msg, int32 will_retain,
						MQTT_PACKET_STRUCTURE *mqttPacket)
{

	uint8 flags = 0;
	uint8 will_topic_len = 0;
	uint16 total_len = 15;
	int16 len = 0, devid_len = strlen(devid);

	if(!devid)
		return 1;

	total_len += devid_len + 2;

	// clean_session: 1=enabled, 0=disabled
	if(clean_session)
	{
		flags |= MQTT_CONNECT_CLEAN_SESSION;
	}

	// will topic: set will flag and accumulate length
	if(will_topic)
	{
		flags |= MQTT_CONNECT_WILL_FLAG;
		will_topic_len = strlen(will_topic);
		total_len += 4 + will_topic_len + strlen(will_msg);
	}

	// QoS level for will message delivery guarantee
	switch((unsigned char)qos)
	{
		case MQTT_QOS_LEVEL0:
			flags |= MQTT_CONNECT_WILL_QOS0;							// at most once
		break;

		case MQTT_QOS_LEVEL1:
			flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_QOS1);	// at least once
		break;

		case MQTT_QOS_LEVEL2:
			flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_QOS2);	// exactly once
		break;

		default:
		return 2;
	}

	// will_retain: broker retains will message for new subscribers
	if(will_retain)
	{
		flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_RETAIN);
	}

	// user and password are required
	if(!user || !password)
	{
		return 3;
	}
	flags |= MQTT_CONNECT_USER_NAME | MQTT_CONNECT_PASSORD;

	total_len += strlen(user) + strlen(password) + 4;

	// allocate buffer
	MQTT_NewBuffer(mqttPacket, total_len);
	if(mqttPacket->_data == NULL)
		return 4;

	memset(mqttPacket->_data, 0, total_len);

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_CONNECT << 4;

	// Fixed header -- remaining length
	len = MQTT_DumpLength(total_len - 5, mqttPacket->_data + mqttPacket->_len);
	if(len < 0)
	{
		MQTT_DeleteBuffer(mqttPacket);
		return 5;
	}
	else
		mqttPacket->_len += len;

/* Variable header */

	// Variable header -- protocol name and version
	mqttPacket->_data[mqttPacket->_len++] = 0;
	mqttPacket->_data[mqttPacket->_len++] = 4;
	mqttPacket->_data[mqttPacket->_len++] = 'M';
	mqttPacket->_data[mqttPacket->_len++] = 'Q';
	mqttPacket->_data[mqttPacket->_len++] = 'T';
	mqttPacket->_data[mqttPacket->_len++] = 'T';

	// Variable header -- protocol level 4
	mqttPacket->_data[mqttPacket->_len++] = 4;

	// Variable header -- connect flags
    mqttPacket->_data[mqttPacket->_len++] = flags;

	// Variable header -- keep-alive interval (seconds)
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(cTime);
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(cTime);

/* Payload */

	// Payload -- devid length and devid
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(devid_len);
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(devid_len);

	strncat((int8 *)mqttPacket->_data + mqttPacket->_len, devid, devid_len);
	mqttPacket->_len += devid_len;

	// Payload -- will topic and will message
	if(flags & MQTT_CONNECT_WILL_FLAG)
	{
		unsigned short mLen = 0;

		if(!will_msg)
			will_msg = "";

		mLen = strlen(will_topic);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(mLen);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(mLen);
		strncat((int8 *)mqttPacket->_data + mqttPacket->_len, will_topic, mLen);
		mqttPacket->_len += mLen;

		mLen = strlen(will_msg);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(mLen);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(mLen);
		strncat((int8 *)mqttPacket->_data + mqttPacket->_len, will_msg, mLen);
		mqttPacket->_len += mLen;
	}

	// Payload -- username
	if(flags & MQTT_CONNECT_USER_NAME)
	{
		unsigned short user_len = strlen(user);

		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(user_len);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(user_len);
		strncat((int8 *)mqttPacket->_data + mqttPacket->_len, user, user_len);
		mqttPacket->_len += user_len;
	}

	// Payload -- password
	if(flags & MQTT_CONNECT_PASSORD)
	{
		unsigned short psw_len = strlen(password);

		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(psw_len);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(psw_len);
		strncat((int8 *)mqttPacket->_data + mqttPacket->_len, password, psw_len);
		mqttPacket->_len += psw_len;
	}

	return 0;

}

/* MQTT_PacketDisConnect: build DISCONNECT packet */
uint1 MQTT_PacketDisConnect(MQTT_PACKET_STRUCTURE *mqttPacket)
{

	MQTT_NewBuffer(mqttPacket, 2);
	if(mqttPacket->_data == NULL)
		return 1;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_DISCONNECT << 4;

	// Fixed header -- remaining length
	mqttPacket->_data[mqttPacket->_len++] = 0;

	return 0;

}

/* MQTT_UnPacketConnectAck: parse CONNACK packet */
uint8 MQTT_UnPacketConnectAck(uint8 *rev_data)
{

	if(rev_data[1] != 2)
		return 1;

	if(rev_data[2] == 0 || rev_data[2] == 1)
		return rev_data[3];
	else
		return 255;

}

/* MQTT_PacketSaveData: build PUBLISH packet for data upload */
uint1 MQTT_PacketSaveData(const int8 *pro_id, const char *dev_name,
								int16 send_len, int8 *type_bin_head, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	char topic_buf[48];

	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/post", pro_id, dev_name);

	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic_buf, NULL, send_len + 0, MQTT_QOS_LEVEL1, 0, 1, mqttPacket) == 0)
	{
//		mqttPacket->_data[mqttPacket->_len++] = type;
//
//		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(send_len);
//		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(send_len);
	}
	else
		return 1;

	return 0;

}

/* MQTT_PacketSaveBinData: build PUBLISH packet for binary file upload */
uint1 MQTT_PacketSaveBinData(const int8 *name, int16 file_len, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	uint1 result = 1;
	int8 *bin_head = NULL;
	uint8 bin_head_len = 0;
	int8 *payload = NULL;
	int32 payload_size = 0;

	bin_head = (int8 *)MQTT_MallocBuffer(13 + strlen(name));
	if(bin_head == NULL)
		return result;

	sprintf(bin_head, "{\"ds_id\":\"%s\"}", name);

	bin_head_len = strlen(bin_head);
	payload_size = 7 + bin_head_len + file_len;

	payload = (int8 *)MQTT_MallocBuffer(payload_size - file_len);
	if(payload == NULL)
	{
		MQTT_FreeBuffer(bin_head);

		return result;
	}

	payload[0] = 2;						// type

	payload[1] = MOSQ_MSB(bin_head_len);
	payload[2] = MOSQ_LSB(bin_head_len);

	memcpy(payload + 3, bin_head, bin_head_len);

	payload[bin_head_len + 3] = (file_len >> 24) & 0xFF;
	payload[bin_head_len + 4] = (file_len >> 16) & 0xFF;
	payload[bin_head_len + 5] = (file_len >> 8) & 0xFF;
	payload[bin_head_len + 6] = file_len & 0xFF;

	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, "$dp", payload, payload_size, MQTT_QOS_LEVEL1, 0, 1, mqttPacket) == 0)
		result = 0;

	MQTT_FreeBuffer(bin_head);
	MQTT_FreeBuffer(payload);

	return result;

}

/* MQTT_UnPacketCmd: parse command topic payload, extract cmdid and request */
uint8 MQTT_UnPacketCmd(uint8 *rev_data, int8 **cmdid, int8 **req, uint16 *req_len)
{

	int8 *dataPtr = strchr((int8 *)rev_data + 6, '/');	// skip 6-byte fixed header

	uint32 remain_len = 0;

	if(dataPtr == NULL)									// '/' not found
		return 1;
	dataPtr++;											// skip '/'

	MQTT_ReadLength(rev_data + 1, 4, &remain_len);		// read remaining length

	*cmdid = (int8 *)MQTT_MallocBuffer(37);				// cmdid is fixed 36 bytes + null terminator
	if(*cmdid == NULL)
		return 2;

	memset(*cmdid, 0, 37);								// zero out
	memcpy(*cmdid, (const int8 *)dataPtr, 36);			// copy cmdid
	dataPtr += 36;

	*req_len = remain_len - 44;							// req_len = remain_len - 2 - 5($creq) - 1(/) - 36(cmdid)
	*req = (int8 *)MQTT_MallocBuffer(*req_len + 1);		// allocate request buffer + null terminator
	if(*req == NULL)
	{
		MQTT_FreeBuffer(*cmdid);
		return 3;
	}

	memset(*req, 0, *req_len + 1);
	memcpy(*req, (const int8 *)dataPtr, *req_len);		// copy request body

	return 0;

}

/* MQTT_PacketCmdResp: build command response PUBLISH packet */
uint1 MQTT_PacketCmdResp(const int8 *cmdid, const int8 *req, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	uint16 cmdid_len = strlen(cmdid);
	uint16 req_len = strlen(req);
	_Bool status = 0;

	int8 *payload = MQTT_MallocBuffer(cmdid_len + 7);
	if(payload == NULL)
		return 1;

	memset(payload, 0, cmdid_len + 7);
	memcpy(payload, "$crsp/", 6);
	strncat(payload, cmdid, cmdid_len);

	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, payload, req, strlen(req), MQTT_QOS_LEVEL0, 0, 1, mqttPacket) == 0)
		status = 0;
	else
		status = 1;

	MQTT_FreeBuffer(payload);

	return status;

}

/* MQTT_PacketSubscribe: build SUBSCRIBE packet */
uint8 MQTT_PacketSubscribe(uint16 pkt_id, enum MqttQosLevel qos, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	uint32 topic_len = 0, remain_len = 0;
	int16 len = 0;
	uint8 i = 0;

	if(pkt_id == 0)
		return 1;

	// accumulate total topic string length
	for(; i < topics_cnt; i++)
	{
		if(topics[i] == NULL)
			return 2;

		topic_len += strlen(topics[i]);
	}

	//2 bytes packet id + topic filter(2 bytes topic + topic length + 1 byte reserve)------
	remain_len = 2 + 3 * topics_cnt + topic_len;

	// allocate buffer
	MQTT_NewBuffer(mqttPacket, remain_len + 5);
	if(mqttPacket->_data == NULL)
		return 3;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_SUBSCRIBE << 4 | 0x02;

	// Fixed header -- remaining length
	len = MQTT_DumpLength(remain_len, mqttPacket->_data + mqttPacket->_len);
	if(len < 0)
	{
		MQTT_DeleteBuffer(mqttPacket);
		return 4;
	}
	else
		mqttPacket->_len += len;

/* Payload */

	//payload----------------------pkt_id---------------------------------------------------
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(pkt_id);
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(pkt_id);

	//payload----------------------topic_name-----------------------------------------------
	for(i = 0; i < topics_cnt; i++)
	{
		topic_len = strlen(topics[i]);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(topic_len);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(topic_len);

		strncat((int8 *)mqttPacket->_data + mqttPacket->_len, topics[i], topic_len);
		mqttPacket->_len += topic_len;

		mqttPacket->_data[mqttPacket->_len++] = qos & 0xFF;
	}

	return 0;

}

/* MQTT_UnPacketSubscribe: parse SUBACK packet */
uint8 MQTT_UnPacketSubscribe(uint8 *rev_data)
{

	uint8 result = 255;

	if(rev_data[2] == MOSQ_MSB(MQTT_SUBSCRIBE_ID) && rev_data[3] == MOSQ_LSB(MQTT_SUBSCRIBE_ID))
	{
		switch(rev_data[4])
		{
			case 0x00:
			case 0x01:
			case 0x02:
				//MQTT Subscribe OK
				result = 0;
			break;

			case 0x80:
				//MQTT Subscribe Failed
				result = 1;
			break;

			default:
				//MQTT Subscribe UnKnown Err
				result = 2;
			break;
		}
	}

	return result;

}

/* MQTT_PacketUnSubscribe: build UNSUBSCRIBE packet */
uint8 MQTT_PacketUnSubscribe(uint16 pkt_id, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	uint32 topic_len = 0, remain_len = 0;
	int16 len = 0;
	uint8 i = 0;

	if(pkt_id == 0)
		return 1;

	// accumulate total topic string length
	for(; i < topics_cnt; i++)
	{
		if(topics[i] == NULL)
			return 2;

		topic_len += strlen(topics[i]);
	}

	//2 bytes packet id, 2 bytes topic length + topic + 1 byte reserve---------------------
	remain_len = 2 + (topics_cnt << 1) + topic_len;

	// allocate buffer
	MQTT_NewBuffer(mqttPacket, remain_len + 5);
	if(mqttPacket->_data == NULL)
		return 3;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_UNSUBSCRIBE << 4 | 0x02;

	// Fixed header -- remaining length
	len = MQTT_DumpLength(remain_len, mqttPacket->_data + mqttPacket->_len);
	if(len < 0)
	{
		MQTT_DeleteBuffer(mqttPacket);
		return 4;
	}
	else
		mqttPacket->_len += len;

/* Payload */

	//payload----------------------pkt_id---------------------------------------------------
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(pkt_id);
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(pkt_id);

	//payload----------------------topic_name-----------------------------------------------
	for(i = 0; i < topics_cnt; i++)
	{
		topic_len = strlen(topics[i]);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(topic_len);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(topic_len);

		strncat((int8 *)mqttPacket->_data + mqttPacket->_len, topics[i], topic_len);
		mqttPacket->_len += topic_len;
	}

	return 0;

}

/* MQTT_UnPacketUnSubscribe: parse UNSUBACK packet */
uint1 MQTT_UnPacketUnSubscribe(uint8 *rev_data)
{

	uint1 result = 1;

	if(rev_data[2] == MOSQ_MSB(MQTT_UNSUBSCRIBE_ID) && rev_data[3] == MOSQ_LSB(MQTT_UNSUBSCRIBE_ID))
	{
		result = 0;
	}

	return result;

}

/* MQTT_PacketPublish: build PUBLISH packet */
uint8 MQTT_PacketPublish(uint16 pkt_id, const int8 *topic,
						const int8 *payload, uint32 payload_len,
						enum MqttQosLevel qos, int32 retain, int32 own,
						MQTT_PACKET_STRUCTURE *mqttPacket)
{

	uint32 total_len = 0, topic_len = 0;
	uint32 data_len = 0;
	int32 len = 0;
	uint8 flags = 0;

	// validate pkt_id
	if(pkt_id == 0)
		return 1;

	// validate topic: wildcard characters not allowed in PUBLISH
	for(topic_len = 0; topic[topic_len] != '\0'; ++topic_len)
	{
		if((topic[topic_len] == '#') || (topic[topic_len] == '+'))
			return 2;
	}

	// set PUBLISH packet type flags
	flags |= MQTT_PKT_PUBLISH << 4;

	// set retain flag
	if(retain)
		flags |= 0x01;

	// total remaining length
	total_len = topic_len + payload_len + 2;

	// QoS level
	switch(qos)
	{
		case MQTT_QOS_LEVEL0:
			flags |= MQTT_CONNECT_WILL_QOS0;	// at most once
		break;

		case MQTT_QOS_LEVEL1:
			flags |= 0x02;						// at least once
			total_len += 2;
		break;

		case MQTT_QOS_LEVEL2:
			flags |= 0x04;						// exactly once
			total_len += 2;
		break;

		default:
		return 3;
	}

	// allocate buffer
	if(payload != NULL)
	{
		if(payload[0] == 2)
		{
			uint32 data_len_t = 0;

			while(payload[data_len_t++] != '}');
			data_len_t -= 3;
			data_len = data_len_t + 7;
			data_len_t = payload_len - data_len;

			MQTT_NewBuffer(mqttPacket, total_len + 3 - data_len_t);

			if(mqttPacket->_data == NULL)
				return 4;

			memset(mqttPacket->_data, 0, total_len + 3 - data_len_t);
		}
		else
		{
			MQTT_NewBuffer(mqttPacket, total_len + 5);

			if(mqttPacket->_data == NULL)
				return 4;

			memset(mqttPacket->_data, 0, total_len + 5);
		}
	}
	else
	{
		MQTT_NewBuffer(mqttPacket, total_len + 5);

		if(mqttPacket->_data == NULL)
			return 4;

		memset(mqttPacket->_data, 0, total_len + 5);
	}

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = flags;

	// Fixed header -- remaining length
	len = MQTT_DumpLength(total_len, mqttPacket->_data + mqttPacket->_len);
	if(len < 0)
	{
		MQTT_DeleteBuffer(mqttPacket);
		return 5;
	}
	else
		mqttPacket->_len += len;

/* Variable header */

	// Variable header -- topic length and topic string
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(topic_len);
	mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(topic_len);

	strncat((int8 *)mqttPacket->_data + mqttPacket->_len, topic, topic_len);
	mqttPacket->_len += topic_len;
	if(qos != MQTT_QOS_LEVEL0)
	{
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_MSB(pkt_id);
		mqttPacket->_data[mqttPacket->_len++] = MOSQ_LSB(pkt_id);
	}

	// Variable header -- write payload
	if(payload != NULL)
	{
		if(payload[0] == 2)
		{
			memcpy((int8 *)mqttPacket->_data + mqttPacket->_len, payload, data_len);
			mqttPacket->_len += data_len;
		}
		else
		{
			memcpy((int8 *)mqttPacket->_data + mqttPacket->_len, payload, payload_len);
			mqttPacket->_len += payload_len;
		}
	}

	return 0;

}

/* MQTT_UnPacketPublish: parse received PUBLISH packet */
uint8 MQTT_UnPacketPublish(uint8 *rev_data, int8 **topic, uint16 *topic_len, int8 **payload, uint16 *payload_len, uint8 *qos, uint16 *pkt_id)
{

	const int8 flags = rev_data[0] & 0x0F;
	uint8 *msgPtr;
	uint32 remain_len = 0;

	const int8 dup = flags & 0x08;

	*qos = (flags & 0x06) >> 1;

	msgPtr = rev_data + MQTT_ReadLength(rev_data + 1, 4, &remain_len) + 1;

	if(remain_len < 2 || flags & 0x01)							//retain
		return 255;

	*topic_len = (uint16)msgPtr[0] << 8 | msgPtr[1];
	if(remain_len < *topic_len + 2)
		return 255;

	if(strstr((int8 *)msgPtr + 2, CMD_TOPIC_PREFIX) != NULL)	// command topic prefix match
		return MQTT_PKT_CMD;

	switch(*qos)
	{
		case MQTT_QOS_LEVEL0:									// qos0 have no packet identifier

			if(0 != dup)
				return 255;

			*topic = MQTT_MallocBuffer(*topic_len + 1);			// allocate topic buffer
			if(*topic == NULL)
				return 255;

			memset(*topic, 0, *topic_len + 1);
			memcpy(*topic, (int8 *)msgPtr + 2, *topic_len);	// copy topic

			*payload_len = remain_len - 2 - *topic_len;
			*payload = MQTT_MallocBuffer(*payload_len + 1);		// allocate payload buffer
			if(*payload == NULL)								// alloc failed
			{
				MQTT_FreeBuffer(*topic);						// free topic on error
				return 255;
			}

			memset(*payload, 0, *payload_len + 1);
			memcpy(*payload, (int8 *)msgPtr + 2 + *topic_len, *payload_len);

		break;

		case MQTT_QOS_LEVEL1:
		case MQTT_QOS_LEVEL2:

			if(*topic_len + 2 > remain_len)
				return 255;

			*pkt_id = (uint16)msgPtr[*topic_len + 2] << 8 | msgPtr[*topic_len + 3];
			if(pkt_id == 0)
				return 255;

			*topic = MQTT_MallocBuffer(*topic_len + 1);			// allocate topic buffer
			if(*topic == NULL)
				return 255;

			memset(*topic, 0, *topic_len + 1);
			memcpy(*topic, (int8 *)msgPtr + 2, *topic_len);	// copy topic

			*payload_len = remain_len - 4 - *topic_len;
			*payload = MQTT_MallocBuffer(*payload_len + 1);		// allocate payload buffer
			if(*payload == NULL)								// alloc failed
			{
				MQTT_FreeBuffer(*topic);						// free topic on error
				return 255;
			}

			memset(*payload, 0, *payload_len + 1);
			memcpy(*payload, (int8 *)msgPtr + 4 + *topic_len, *payload_len);

		break;

		default:
			return 255;
	}

	if(strchr((int8 *)topic, '+') || strchr((int8 *)topic, '#'))
		return 255;

	return 0;

}

/* MQTT_PacketPublishAck: build PUBACK packet (QoS 1 reply) */
uint1 MQTT_PacketPublishAck(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	MQTT_NewBuffer(mqttPacket, 4);
	if(mqttPacket->_data == NULL)
		return 1;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_PUBACK << 4;

	// Fixed header -- remaining length
	mqttPacket->_data[mqttPacket->_len++] = 2;

/* Variable header */

	// Variable header -- packet identifier
	mqttPacket->_data[mqttPacket->_len++] = pkt_id >> 8;
	mqttPacket->_data[mqttPacket->_len++] = pkt_id & 0xff;

	return 0;

}

/* MQTT_UnPacketPublishAck: parse PUBACK packet */
uint1 MQTT_UnPacketPublishAck(uint8 *rev_data)
{

	if(rev_data[1] != 2)
		return 1;

	if(rev_data[2] == MOSQ_MSB(MQTT_PUBLISH_ID) && rev_data[3] == MOSQ_LSB(MQTT_PUBLISH_ID))
		return 0;
	else
		return 1;

}

/* MQTT_PacketPublishRec: build PUBREC packet (QoS 2 step 1) */
uint1 MQTT_PacketPublishRec(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	MQTT_NewBuffer(mqttPacket, 4);
	if(mqttPacket->_data == NULL)
		return 1;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_PUBREC << 4;

	// Fixed header -- remaining length
	mqttPacket->_data[mqttPacket->_len++] = 2;

/* Variable header */

	// Variable header -- packet identifier
	mqttPacket->_data[mqttPacket->_len++] = pkt_id >> 8;
	mqttPacket->_data[mqttPacket->_len++] = pkt_id & 0xff;

	return 0;

}

/* MQTT_UnPacketPublishRec: parse PUBREC packet */
uint1 MQTT_UnPacketPublishRec(uint8 *rev_data)
{

	if(rev_data[1] != 2)
		return 1;

	if(rev_data[2] == MOSQ_MSB(MQTT_PUBLISH_ID) && rev_data[3] == MOSQ_LSB(MQTT_PUBLISH_ID))
		return 0;
	else
		return 1;

}

/* MQTT_PacketPublishRel: build PUBREL packet (QoS 2 step 2) */
uint1 MQTT_PacketPublishRel(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	MQTT_NewBuffer(mqttPacket, 4);
	if(mqttPacket->_data == NULL)
		return 1;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_PUBREL << 4 | 0x02;

	// Fixed header -- remaining length
	mqttPacket->_data[mqttPacket->_len++] = 2;

/* Variable header */

	// Variable header -- packet identifier
	mqttPacket->_data[mqttPacket->_len++] = pkt_id >> 8;
	mqttPacket->_data[mqttPacket->_len++] = pkt_id & 0xff;

	return 0;

}

/* MQTT_UnPacketPublishRel: parse PUBREL packet */
uint1 MQTT_UnPacketPublishRel(uint8 *rev_data, uint16 pkt_id)
{

	if(rev_data[1] != 2)
		return 1;

	if(rev_data[2] == MOSQ_MSB(pkt_id) && rev_data[3] == MOSQ_LSB(pkt_id))
		return 0;
	else
		return 1;

}

/* MQTT_PacketPublishComp: build PUBCOMP packet (QoS 2 step 3) */
uint1 MQTT_PacketPublishComp(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket)
{

	MQTT_NewBuffer(mqttPacket, 4);
	if(mqttPacket->_data == NULL)
		return 1;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_PUBCOMP << 4;

	// Fixed header -- remaining length
	mqttPacket->_data[mqttPacket->_len++] = 2;

/* Variable header */

	// Variable header -- packet identifier
	mqttPacket->_data[mqttPacket->_len++] = pkt_id >> 8;
	mqttPacket->_data[mqttPacket->_len++] = pkt_id & 0xff;

	return 0;

}

/* MQTT_UnPacketPublishComp: parse PUBCOMP packet */
uint1 MQTT_UnPacketPublishComp(uint8 *rev_data)
{

	if(rev_data[1] != 2)
		return 1;

	if(rev_data[2] == MOSQ_MSB(MQTT_PUBLISH_ID) && rev_data[3] == MOSQ_LSB(MQTT_PUBLISH_ID))
		return 0;
	else
		return 1;

}

/* MQTT_PacketPing: build PINGREQ packet */
uint1 MQTT_PacketPing(MQTT_PACKET_STRUCTURE *mqttPacket)
{

	MQTT_NewBuffer(mqttPacket, 2);
	if(mqttPacket->_data == NULL)
		return 1;

/* Fixed header */

	// Fixed header -- packet type byte
	mqttPacket->_data[mqttPacket->_len++] = MQTT_PKT_PINGREQ << 4;

	// Fixed header -- remaining length
	mqttPacket->_data[mqttPacket->_len++] = 0;

	return 0;

}
