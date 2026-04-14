//
// HALEspCore — system-level methods (delay, timers, GPIO / SPI init)
//

#include "hal_esp32s3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

void HALEspCore::init() {
    _gpio_init();
    _spi_init();
    _u8g2_init();
}

// ---------------------------------------------------------------------------
// GPIO
// ---------------------------------------------------------------------------

void HALEspCore::_gpio_init() {
    // DC and RST as outputs
    gpio_config_t io = {};
    io.mode          = GPIO_MODE_OUTPUT;
    io.pin_bit_mask  = (1ULL << PIN_OLED_DC) | (1ULL << PIN_OLED_RST);
    io.pull_up_en    = GPIO_PULLUP_DISABLE;
    io.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    io.intr_type     = GPIO_INTR_DISABLE;
    gpio_config(&io);

    // KEY0 and KEY1 as inputs with pull-up (active low)
    io.mode         = GPIO_MODE_INPUT;
    io.pull_up_en   = GPIO_PULLUP_ENABLE;
    io.pin_bit_mask = (1ULL << PIN_KEY0) | (1ULL << PIN_KEY1);
    gpio_config(&io);
}

// ---------------------------------------------------------------------------
// SPI
// ---------------------------------------------------------------------------

void HALEspCore::_spi_init() {
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num    = PIN_OLED_MOSI;
    buscfg.miso_io_num    = -1;
    buscfg.sclk_io_num    = PIN_OLED_CLK;
    buscfg.quadwp_io_num  = -1;
    buscfg.quadhd_io_num  = -1;
    buscfg.max_transfer_sz = 128 * 8;  // full frame buffer
    spi_bus_initialize(OLED_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 8 * 1000 * 1000;  // 8 MHz
    devcfg.mode           = 0;
    devcfg.spics_io_num   = PIN_OLED_CS;
    devcfg.queue_size     = 7;
    spi_bus_add_device(OLED_SPI_HOST, &devcfg, &spiDev);
}

// ---------------------------------------------------------------------------
// System timers
// ---------------------------------------------------------------------------

void HALEspCore::_delay(unsigned long _mill) {
    vTaskDelay(pdMS_TO_TICKS(_mill));
}

unsigned long HALEspCore::_millis() {
    // Use esp_timer for a tick-rate-independent millisecond counter.
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

unsigned long HALEspCore::_getTick() {
    // esp_timer_get_time() returns microseconds; return as microseconds for
    // highest precision (same semantic as the STM32 implementation)
    return (unsigned long)(esp_timer_get_time());
}

unsigned long HALEspCore::_getRandomSeed() {
    return (unsigned long)esp_random();
}
