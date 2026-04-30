#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes queues and state only — does NOT create the task.
 * Call before lcd_touch_init() / ui_main_init(). */
esp_err_t assistant_init(void);

/* Trigger a session. Creates the task on first call.
 * Enforces 3 s minimum retry interval after ERROR. */
void assistant_start_listening(void);

#ifdef __cplusplus
}
#endif
