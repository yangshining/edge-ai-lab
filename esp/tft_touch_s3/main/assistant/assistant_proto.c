#include "assistant_proto.h"
#include <string.h>
#include <arpa/inet.h>   /* htons / htonl */
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "proto";

/* Binary Protocol 2 header (network byte order, 16 bytes):
 * uint16 version | uint16 type | uint32 reserved | uint32 timestamp | uint32 payload_size */
typedef struct __attribute__((packed)) {
    uint16_t version;
    uint16_t type;
    uint32_t reserved;
    uint32_t timestamp_ms;
    uint32_t payload_size;
} bp2_header_t;

size_t proto_pack_audio(uint8_t *out, size_t out_size,
                        const uint8_t *opus, size_t opus_len,
                        uint32_t timestamp_ms)
{
    size_t total = sizeof(bp2_header_t) + opus_len;
    if (out_size < total) return 0;
    bp2_header_t *h = (bp2_header_t *)out;
    h->version      = htons(2);
    h->type         = htons(0);   /* 0 = audio */
    h->reserved     = 0;
    h->timestamp_ms = htonl(timestamp_ms);
    h->payload_size = htonl((uint32_t)opus_len);
    memcpy(out + sizeof(bp2_header_t), opus, opus_len);
    return total;
}

void proto_make_hello(char *buf, size_t size)
{
    snprintf(buf, size,
        "{\"type\":\"hello\",\"version\":2,"
        "\"transport\":\"websocket\","
        "\"audio_params\":{\"format\":\"opus\","
        "\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}");
}

void proto_make_listen_start(char *buf, size_t size, const char *session_id)
{
    snprintf(buf, size,
        "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"auto\"}",
        session_id ? session_id : "");
}

void proto_make_listen_stop(char *buf, size_t size, const char *session_id)
{
    snprintf(buf, size,
        "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}",
        session_id ? session_id : "");
}

void proto_make_abort(char *buf, size_t size, const char *session_id)
{
    snprintf(buf, size,
        "{\"session_id\":\"%s\",\"type\":\"abort\"}",
        session_id ? session_id : "");
}

server_msg_type_t proto_parse_server_msg(const char *json,
                                          char *session_id_out, size_t sid_size,
                                          char *text_out, size_t text_size)
{
    if (!json) return SERVER_MSG_UNKNOWN;
    cJSON *root = cJSON_Parse(json);
    if (!root) return SERVER_MSG_UNKNOWN;

    server_msg_type_t result = SERVER_MSG_UNKNOWN;

    cJSON *type_j = cJSON_GetObjectItem(root, "type");
    cJSON *sid_j  = cJSON_GetObjectItem(root, "session_id");
    cJSON *text_j = cJSON_GetObjectItem(root, "text");

    if (sid_j && cJSON_IsString(sid_j) && session_id_out)
        strlcpy(session_id_out, sid_j->valuestring, sid_size);
    if (text_j && cJSON_IsString(text_j) && text_out)
        strlcpy(text_out, text_j->valuestring, text_size);

    if (type_j && cJSON_IsString(type_j)) {
        const char *t = type_j->valuestring;
        if (strcmp(t, "hello") == 0) {
            result = SERVER_MSG_HELLO;
        } else if (strcmp(t, "stt") == 0) {
            result = SERVER_MSG_STT;
        } else if (strcmp(t, "tts") == 0) {
            cJSON *state_j = cJSON_GetObjectItem(root, "state");
            if (state_j && cJSON_IsString(state_j)) {
                if (strcmp(state_j->valuestring, "start") == 0)
                    result = SERVER_MSG_TTS_START;
                else if (strcmp(state_j->valuestring, "stop") == 0
                         || strcmp(state_j->valuestring, "end") == 0)
                    result = SERVER_MSG_TTS_END;
            }
        }
    }

    cJSON_Delete(root);
    return result;
}
