#include "assistant_state.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_lock;
static assistant_state_t s_state;

void assistant_state_init(void)
{
    if (!s_lock) s_lock = xSemaphoreCreateMutex();
    memset(&s_state, 0, sizeof(s_state));
    s_state.status = ASSISTANT_IDLE;
}

void assistant_state_get(assistant_state_t *out)
{
    if (!out) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_state;
    xSemaphoreGive(s_lock);
}

void assistant_state_set_status(assistant_status_t status)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_state.status = status;
    xSemaphoreGive(s_lock);
}

void assistant_state_set_session(const char *session_id)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(s_state.session_id, session_id ? session_id : "", sizeof(s_state.session_id));
    xSemaphoreGive(s_lock);
}

void assistant_state_set_reply(const char *text)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(s_state.last_reply, text ? text : "", sizeof(s_state.last_reply));
    xSemaphoreGive(s_lock);
}

void assistant_state_set_error(esp_err_t err)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_state.status     = ASSISTANT_ERROR;
    s_state.last_error = err;
    xSemaphoreGive(s_lock);
}
