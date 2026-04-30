#include "assistant_proto.h"
#include <string.h>
#include <stdio.h>
#include "cJSON.h"

void proto_make_hello(char *buf, size_t size)
{
    snprintf(buf, size,
        "{\"type\":\"hello\",\"version\":1,"
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
