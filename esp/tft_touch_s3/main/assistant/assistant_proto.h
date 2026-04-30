#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SERVER_MSG_HELLO,
    SERVER_MSG_STT,
    SERVER_MSG_TTS_START,
    SERVER_MSG_TTS_END,
    SERVER_MSG_UNKNOWN,
} server_msg_type_t;

void proto_make_hello(char *buf, size_t size);
void proto_make_listen_start(char *buf, size_t size, const char *session_id);
void proto_make_listen_stop(char *buf, size_t size, const char *session_id);
void proto_make_abort(char *buf, size_t size, const char *session_id);

/* Parse a JSON string from the server.
 * Fills session_id_out and text_out if present; both may be NULL if not needed.
 * Returns message type. */
server_msg_type_t proto_parse_server_msg(const char *json,
                                          char *session_id_out, size_t sid_size,
                                          char *text_out, size_t text_size);

#ifdef __cplusplus
}
#endif
