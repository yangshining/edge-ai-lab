#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ASSISTANT_IDLE,
    ASSISTANT_CONNECTING,
    ASSISTANT_LISTENING,
    ASSISTANT_UPLOADING,
    ASSISTANT_THINKING,
    ASSISTANT_SPEAKING,
    ASSISTANT_ERROR,
    ASSISTANT_STATE_COUNT,
} assistant_status_t;

typedef struct {
    assistant_status_t status;
    char session_id[64];
    char last_reply[128];
    esp_err_t last_error;
} assistant_state_t;

void assistant_state_init(void);
void assistant_state_get(assistant_state_t *out);
void assistant_state_set_status(assistant_status_t status);
void assistant_state_set_session(const char *session_id);
void assistant_state_set_reply(const char *text);
void assistant_state_set_error(esp_err_t err);

#ifdef __cplusplus
}
#endif
