//
// ESP32-S3 entry point for oled-ui-astra
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "astra_rocket.h"

static void astra_task(void *) {
    astraCoreInit();
    astraCoreStart();   // infinite loop — never returns
    vTaskDelete(nullptr);
}

extern "C" void app_main() {
    xTaskCreatePinnedToCore(
        astra_task,
        "astra",
        16384,   // STL containers + menu tree; verify with uxTaskGetStackHighWaterMark
        nullptr,
        5,
        nullptr,
        APP_CPU_NUM
    );
}
