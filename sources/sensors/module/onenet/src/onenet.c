/** @file onenet.c
 * @brief OneNET platform MQTT interface (auth, connect, publish, subscribe)
 */

/* MCU */
#include "stm32f10x.h"

/* Network device */
#include "esp8266.h"

/* Protocol */
#include "onenet.h"
#include "mqttkit.h"

/* Algorithms */
#include "base64.h"
#include "hmac_sha1.h"

/* Hardware drivers */
#include "usart.h"
#include "delay.h"
/* MODULE CHANGE: beep.h/fan.h/mq.h replaced with stub_actuators.h for portability */
#include "stub_actuators.h"

/* C library */
#include <string.h>
#include <stdio.h>


#include "cJSON.h"

#define PROID			"2b785VX4e7"  //y8cqBsSLs0

#define ACCESS_KEY		"ZUtVM0FaSFdwSTRhbW93WDcxVkJkVXQ0VnV2MXJIdkY=" //WHRFZDU2d2tQOUlWd1Z0WU1kOUVCQ3Jyb1MxbDdrT0E=

#define DEVICE_NAME		"konhqi12121"

/* Comment out to disable MQ2/MQ3/MQ4/MQ7 upload to OneNET */
#define UPLOAD_MQ_DATA


char devid[16];

char key[48];


extern volatile unsigned char esp8266_buf[512];


/*
************************************************************
*	Function Name:	OTA_UrlEncode
*
*	Function:		URL encoding for sign
*
*	Input:			sign - encrypted result
*
*	Return:			0-success	other-failure
*
*	Description:	+			%2B
*					space		%20
*					/			%2F
*					?			%3F
*					%			%25
*					#			%23
*					&			%26
*					=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);

	if(sign == (void *)0 || sign_len < 28)
		return 1;

	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;

	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;

			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;

			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;

			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;

			case '%':
				strcat(sign + j, "%25");j += 3;
			break;

			case '#':
				strcat(sign + j, "%23");j += 3;
			break;

			case '&':
				strcat(sign + j, "%26");j += 3;
			break;

			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;

			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}

	sign[j] = 0;

	return 0;

}

/*
************************************************************
*	Function Name:	OTA_Authorization
*
*	Function:		Generate Authorization
*
*	Input:			ver - protocol version number, fixed format, currently supports format "2018-10-31"
*					res - product id
*					et - expiration time, UTC value
*					access_key - access key
*					dev_name - device name
*					authorization_buf - buffer pointer for generated token
*					authorization_buf_len - buffer size (bytes)
*
*	Return:			0-success	other-failure
*
*	Description:	Currently only supports sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{

	size_t olen = 0;

	char sign_buf[64];								// Store signature Base64 encoded and URL encoded
	char hmac_sha1_buf[64];							// Store signature
	char access_key_base64[64];						// Store access_key Base64 decoded
	char string_for_signature[72];					// Store string_for_signature, which is the encrypted key

	/* Parameter validity check */
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;

	/* Base64 decode access_key */
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
//	UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);

	/* Generate string_for_signature */
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
//	UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);

	/* HMAC-SHA1 encryption */
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));

	/* MODULE CHANGE: use olen (decoded byte count) instead of strlen to handle binary key bytes */
	hmac_sha1((unsigned char *)access_key_base64, (int)olen,
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);

//	UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);

	/* Base64 encode the encrypted result */
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	/* MODULE CHANGE: use fixed 20 (HMAC-SHA1 digest length) instead of strlen to handle binary digest bytes */
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, 20);

	/* URL encode the Base64 result */
	OTA_UrlEncode(sign_buf);
//	UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);

	/* Generate token */
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
//	UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);

	return 0;

}

/* OneNET_RegisterDevice: register device and retrieve devid/key */
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;

	char authorization_buf[144];													// Encrypted key

	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;

	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		DelayXms(500);

	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);

	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",

					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);

	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));

	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "onenet_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",

	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/

	data_ptr = (char *)ESP8266_GetIPD(250);							// Wait for platform response

	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}

	if(data_ptr)
	{
		char name[16];
		int pid = 0;

		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			UsartPrintf(USART_DEBUG, "create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}

	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");

	return result;

}

/* OneNet_DevLink: connect to OneNET platform via MQTT; returns 0 on success */
_Bool OneNet_DevLink(void)
{

	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					// Protocol packet

	unsigned char *dataPtr;

	char authorization_buf[160];

	_Bool status = 1;

	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0);

	UsartPrintf(USART_DEBUG, "OneNET_DevLink\r\n"
							"NAME: %s,	PROID: %s,	KEY:%s\r\n"
                        , DEVICE_NAME, PROID, authorization_buf);

	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 1000, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			// Upload to platform

		dataPtr = ESP8266_GetIPD(250);									// Wait for platform response
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:UsartPrintf(USART_DEBUG, "Tips:	Connection successful\r\n");status = 0;break;

					case 1:UsartPrintf(USART_DEBUG, "WARN:	Connection failed: Protocol error\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	Connection failed: Illegal clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	Connection failed: Server failure\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	Connection failed: Username or password error\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	Connection failed: Unauthorized (check token validity)\r\n");break;

					default:UsartPrintf(USART_DEBUG, "ERR:	Connection failed: Unknown error\r\n");break;
				}
			}
		}

		MQTT_DeleteBuffer(&mqttPacket);								// Delete
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");

	return status;

}
extern u8  temp, humi, temp_adj, humi_adj;
extern u8  fan_mode, led_mode;
extern int light_pct, light_th;
/* mq2_vol / smog_th: defined in demo_main.c for symbol resolution; not used in upload logic */
extern int mq2_vol, smog_th;

unsigned char OneNet_FillBuf(char *buf)
{

	char text[100];

	memset(text, 0, sizeof(text));

	strcpy(buf, "{\"id\":\"123\",\"params\":{");

	memset(text, 0, sizeof(text));
	sprintf(text, "\"temp\":{\"value\":%d},", temp);
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text, "\"humi\":{\"value\":%d},", humi);
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text, "\"light\":{\"value\":%d},", light_pct);
	strcat(buf, text);

#ifdef UPLOAD_MQ_DATA
	memset(text, 0, sizeof(text));
	sprintf(text, "\"mq2\":{\"value\":%d},", Mq2_GetPercentage());
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text, "\"mq3\":{\"value\":%d},", Mq3_GetPercentage());
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text, "\"mq4\":{\"value\":%d},", Mq4_GetPercentage());
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text, "\"mq7\":{\"value\":%d},", Mq7_GetPercentage());
	strcat(buf, text);
#endif

	memset(text, 0, sizeof(text));
	sprintf(text, "\"beep\":{\"value\":%s}", Beep_Status ? "true" : "false");
	strcat(buf, text);

	strcat(buf, "}}");

	return strlen(buf);

}

/* OneNet_SendData: build JSON payload and publish to OneNET */
void OneNet_SendData(void)
{

	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												// Protocol packet

	char buf[384];

	short body_len = 0, i = 0;

//	UsartPrintf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");

	memset(buf, 0, sizeof(buf));

	body_len = OneNet_FillBuf(buf);																	// Get current data to be sent and total length

	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				// Encapsulation
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];

			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									// Upload data to platform
//			UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqttPacket._len);

			MQTT_DeleteBuffer(&mqttPacket);															// Delete
		}
		else
			UsartPrintf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n");
	}

}

/* OneNET_Publish: publish a message to the given MQTT topic */
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						// Protocol packet

	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);

	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					// Send subscription message to platform

		MQTT_DeleteBuffer(&mqtt_packet);										// Delete
	}

}

/* OneNET_Subscribe: subscribe to the thing/property/set topic */
void OneNET_Subscribe(void)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						// Protocol packet

	char topic_buf[56];
	const char *topic = topic_buf;

	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);

	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);

	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					// Send subscription message to platform

		MQTT_DeleteBuffer(&mqtt_packet);										// Delete
	}

}

/* OneNet_RevPro: process incoming MQTT packets from platform */
void OneNet_RevPro(unsigned char *cmd)
{

	char *req_payload = NULL;
	char *cmdid_topic = NULL;

	unsigned short topic_len = 0;
	unsigned short req_len = 0;

	unsigned char qos = 0;
	static unsigned short pkt_id = 0;

	unsigned char type = 0;

	short result = 0;

	cJSON *raw_json, *params_json, *fan_json, *light_th_json, *temp_json, *humi_json, *led_json;

	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																// Received Publish message

			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
								cmdid_topic, topic_len, req_payload, req_len);

				raw_json = cJSON_Parse(req_payload);
				if(raw_json == NULL)
				{
					UsartPrintf(USART_DEBUG, "ERR: cJSON_Parse failed\r\n");
					break;
				}
				params_json = cJSON_GetObjectItem(raw_json, "params");
				if(params_json == NULL)
				{
					UsartPrintf(USART_DEBUG, "ERR: no 'params' in JSON\r\n");
					cJSON_Delete(raw_json);
					break;
				}

				fan_json = cJSON_GetObjectItem(params_json, "fan");
				if(fan_json != NULL)
				{
					if(fan_json->type == cJSON_True) { Fan_Set(FAN_ON);  fan_mode = 3; }
					else                             { Fan_Set(FAN_OFF); fan_mode = 3; }
				}

				led_json = cJSON_GetObjectItem(params_json, "led");
				if(led_json != NULL)
				{
					if(led_json->type == cJSON_True) { Led_Set(LED_ON);  led_mode = 3; }
					else                             { Led_Set(LED_OFF); led_mode = 3; }
				}

				light_th_json = cJSON_GetObjectItem(params_json, "light_th");
				if(light_th_json != NULL)
					light_th = light_th_json->valueint;

				temp_json = cJSON_GetObjectItem(params_json, "temp_adj");
				if(temp_json != NULL)
					temp_adj = (u8)temp_json->valueint;

				humi_json = cJSON_GetObjectItem(params_json, "humi_adj");
				if(humi_json != NULL)
					humi_adj = (u8)humi_json->valueint;

				cJSON_Delete(raw_json);

			}
			break;   /* end MQTT_PKT_PUBLISH — do not fall through */

		case MQTT_PKT_PUBACK:														// After publishing Publish message, platform replies with Ack

			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");

		break;

		case MQTT_PKT_SUBACK:																// After publishing Subscribe message, Ack

			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");

		break;

		default:
			result = -1;
		break;
	}

	ESP8266_Clear();									// Clear buffer

	if(result == -1)
		return;

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}
