//
// HALEspCore — OLED / u8g2 methods
//
// SSD1306 128x64 driven over SPI via ESP-IDF spi_master + u8g2
//

#include <cmath>
#include "hal_esp32s3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

// ---------------------------------------------------------------------------
// u8g2 SPI byte callback
// ---------------------------------------------------------------------------

uint8_t HALEspCore::u8x8_byte_spi_cb(u8x8_t *u8x8,
                                      uint8_t msg,
                                      uint8_t argInt,
                                      void   *argPtr) {
    // Retrieve the instance set in _u8g2_init() via user_ptr.
    // HAL::hal is not yet assigned when this fires during u8g2_InitDisplay(),
    // so we cannot use HAL::get() here.
    HALEspCore *self = static_cast<HALEspCore *>(u8x8->user_ptr);

    switch (msg) {
        case U8X8_MSG_BYTE_SEND: {
            spi_transaction_t t = {};
            t.length    = (size_t)argInt * 8;  // bits
            t.tx_buffer = argPtr;
            spi_device_polling_transmit(self->spiDev, &t);
            break;
        }
        case U8X8_MSG_BYTE_SET_DC:
            gpio_set_level(PIN_OLED_DC, argInt);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8,
                                    U8X8_MSG_DELAY_NANO,
                                    u8x8->display_info->post_chip_enable_wait_ns,
                                    nullptr);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8->gpio_and_delay_cb(u8x8,
                                    U8X8_MSG_DELAY_NANO,
                                    u8x8->display_info->pre_chip_disable_wait_ns,
                                    nullptr);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;
        case U8X8_MSG_BYTE_INIT:
            break;
        default:
            return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// u8g2 GPIO / delay callback
// ---------------------------------------------------------------------------

uint8_t HALEspCore::u8x8_gpio_delay_cb(u8x8_t * /*u8x8*/,
                                        uint8_t  msg,
                                        uint8_t  argInt,
                                        void    * /*argPtr*/) {
    switch (msg) {
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(argInt));
            break;
        case U8X8_MSG_DELAY_NANO:
            // A nano-second delay — a single NOP is fine on a 240 MHz core
            __asm__ volatile ("nop");
            break;
        case U8X8_MSG_GPIO_CS:
            gpio_set_level(PIN_OLED_CS, argInt);
            break;
        case U8X8_MSG_GPIO_DC:
            gpio_set_level(PIN_OLED_DC, argInt);
            break;
        case U8X8_MSG_GPIO_RESET:
            gpio_set_level(PIN_OLED_RST, argInt);
            break;
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;
        default:
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// u8g2 init
// ---------------------------------------------------------------------------

void HALEspCore::_u8g2_init() {
    // Hardware reset sequence
    gpio_set_level(PIN_OLED_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_OLED_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set user_ptr BEFORE u8g2_Setup so the SPI callback can retrieve 'this'
    // when u8g2_InitDisplay() fires callbacks during _u8g2_init().
    canvasBuffer.u8x8.user_ptr = this;

    u8g2_Setup_ssd1306_128x64_noname_f(&canvasBuffer,
                                       U8G2_R0,
                                       u8x8_byte_spi_cb,
                                       u8x8_gpio_delay_cb);
    u8g2_InitDisplay(&canvasBuffer);
    u8g2_SetPowerSave(&canvasBuffer, 0);  // display on
    u8g2_ClearBuffer(&canvasBuffer);

    u8g2_SetFontMode(&canvasBuffer, 1);
    u8g2_SetFontDirection(&canvasBuffer, 0);
    u8g2_SetFont(&canvasBuffer, u8g2_font_myfont);
}

// ---------------------------------------------------------------------------
// Screen on / off
// ---------------------------------------------------------------------------

void HALEspCore::_screenOn() {
    u8g2_SetPowerSave(&canvasBuffer, 0);
}

void HALEspCore::_screenOff() {
    u8g2_SetPowerSave(&canvasBuffer, 1);
}

// ---------------------------------------------------------------------------
// Canvas accessors
// ---------------------------------------------------------------------------

void *HALEspCore::_getCanvasBuffer() {
    return u8g2_GetBufferPtr(&canvasBuffer);
}

unsigned char HALEspCore::_getBufferTileHeight() {
    return u8g2_GetBufferTileHeight(&canvasBuffer);
}

unsigned char HALEspCore::_getBufferTileWidth() {
    return u8g2_GetBufferTileWidth(&canvasBuffer);
}

void HALEspCore::_canvasUpdate() {
    u8g2_SendBuffer(&canvasBuffer);
}

void HALEspCore::_canvasClear() {
    u8g2_ClearBuffer(&canvasBuffer);
}

// ---------------------------------------------------------------------------
// Font
// ---------------------------------------------------------------------------

void HALEspCore::_setFont(const unsigned char *_font) {
    u8g2_SetFontMode(&canvasBuffer, 1);
    u8g2_SetFontDirection(&canvasBuffer, 0);
    u8g2_SetFont(&canvasBuffer, _font);
}

unsigned char HALEspCore::_getFontWidth(std::string &_text) {
    return u8g2_GetUTF8Width(&canvasBuffer, _text.c_str());
}

unsigned char HALEspCore::_getFontHeight() {
    return u8g2_GetMaxCharHeight(&canvasBuffer);
}

// ---------------------------------------------------------------------------
// Draw type
// ---------------------------------------------------------------------------

void HALEspCore::_setDrawType(unsigned char _type) {
    u8g2_SetDrawColor(&canvasBuffer, _type);
}

// ---------------------------------------------------------------------------
// Draw primitives  (float → int16_t via round, identical to STM32 HAL)
// ---------------------------------------------------------------------------

void HALEspCore::_drawPixel(float _x, float _y) {
    u8g2_DrawPixel(&canvasBuffer,
                   (int16_t)std::round(_x),
                   (int16_t)std::round(_y));
}

void HALEspCore::_drawEnglish(float _x, float _y, const std::string &_text) {
    u8g2_DrawStr(&canvasBuffer,
                 (int16_t)std::round(_x),
                 (int16_t)std::round(_y),
                 _text.c_str());
}

void HALEspCore::_drawChinese(float _x, float _y, const std::string &_text) {
    u8g2_DrawUTF8(&canvasBuffer,
                  (int16_t)std::round(_x),
                  (int16_t)std::round(_y),
                  _text.c_str());
}

void HALEspCore::_drawVDottedLine(float _x, float _y, float _h) {
    for (unsigned char i = 0; i < (unsigned char)std::round(_h); i++) {
        if (i % 8 == 0 || (i - 1) % 8 == 0 || (i - 2) % 8 == 0) continue;
        u8g2_DrawPixel(&canvasBuffer,
                       (int16_t)std::round(_x),
                       (int16_t)std::round(_y) + i);
    }
}

void HALEspCore::_drawHDottedLine(float _x, float _y, float _l) {
    for (unsigned char i = 0; i < (unsigned char)std::round(_l); i++) {
        if (i % 8 == 0 || (i - 1) % 8 == 0 || (i - 2) % 8 == 0) continue;
        u8g2_DrawPixel(&canvasBuffer,
                       (int16_t)std::round(_x) + i,
                       (int16_t)std::round(_y));
    }
}

void HALEspCore::_drawVLine(float _x, float _y, float _h) {
    u8g2_DrawVLine(&canvasBuffer,
                   (int16_t)std::round(_x),
                   (int16_t)std::round(_y),
                   (int16_t)std::round(_h));
}

void HALEspCore::_drawHLine(float _x, float _y, float _l) {
    u8g2_DrawHLine(&canvasBuffer,
                   (int16_t)std::round(_x),
                   (int16_t)std::round(_y),
                   (int16_t)std::round(_l));
}

void HALEspCore::_drawBMP(float _x, float _y, float _w, float _h,
                           const unsigned char *_bitMap) {
    u8g2_DrawXBMP(&canvasBuffer,
                  (int16_t)std::round(_x),
                  (int16_t)std::round(_y),
                  (int16_t)std::round(_w),
                  (int16_t)std::round(_h),
                  _bitMap);
}

void HALEspCore::_drawBox(float _x, float _y, float _w, float _h) {
    u8g2_DrawBox(&canvasBuffer,
                 (int16_t)std::round(_x),
                 (int16_t)std::round(_y),
                 (int16_t)std::round(_w),
                 (int16_t)std::round(_h));
}

void HALEspCore::_drawRBox(float _x, float _y, float _w, float _h, float _r) {
    u8g2_DrawRBox(&canvasBuffer,
                  (int16_t)std::round(_x),
                  (int16_t)std::round(_y),
                  (int16_t)std::round(_w),
                  (int16_t)std::round(_h),
                  (int16_t)std::round(_r));
}

void HALEspCore::_drawFrame(float _x, float _y, float _w, float _h) {
    u8g2_DrawFrame(&canvasBuffer,
                   (int16_t)std::round(_x),
                   (int16_t)std::round(_y),
                   (int16_t)std::round(_w),
                   (int16_t)std::round(_h));
}

void HALEspCore::_drawRFrame(float _x, float _y, float _w, float _h, float _r) {
    u8g2_DrawRFrame(&canvasBuffer,
                    (int16_t)std::round(_x),
                    (int16_t)std::round(_y),
                    (int16_t)std::round(_w),
                    (int16_t)std::round(_h),
                    (int16_t)std::round(_r));
}
