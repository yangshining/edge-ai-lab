//
// HALEspCore — ESP32-S3 concrete HAL implementation for oled-ui-astra
//

#pragma once

#include "hal/hal.h"
#include "u8g2/u8g2.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// ---------------------------------------------------------------------------
// Pin definitions — modify to match your wiring
// ---------------------------------------------------------------------------
#define PIN_OLED_MOSI  GPIO_NUM_11
#define PIN_OLED_CLK   GPIO_NUM_12
#define PIN_OLED_CS    GPIO_NUM_10
#define PIN_OLED_DC    GPIO_NUM_9
#define PIN_OLED_RST   GPIO_NUM_8
#define PIN_KEY0       GPIO_NUM_0
#define PIN_KEY1       GPIO_NUM_1
#define OLED_SPI_HOST  SPI2_HOST

class HALEspCore : public HAL {
private:
    u8g2_t        canvasBuffer{};
    spi_device_handle_t spiDev{};

    void _gpio_init();
    void _spi_init();
    void _u8g2_init();

    static uint8_t u8x8_byte_spi_cb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt, void *argPtr);
    static uint8_t u8x8_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt, void *argPtr);

public:
    HALEspCore() = default;

    std::string type() override { return "HALEspCore"; }

    void init() override;

    // ---------- canvas ----------
    void *_getCanvasBuffer() override;
    unsigned char _getBufferTileHeight() override;
    unsigned char _getBufferTileWidth() override;
    void _canvasUpdate() override;
    void _canvasClear() override;

    // ---------- font / draw ----------
    void _setFont(const unsigned char *_font) override;
    unsigned char _getFontWidth(std::string &_text) override;
    unsigned char _getFontHeight() override;
    void _setDrawType(unsigned char _type) override;
    void _drawPixel(float _x, float _y) override;
    void _drawEnglish(float _x, float _y, const std::string &_text) override;
    void _drawChinese(float _x, float _y, const std::string &_text) override;
    void _drawVDottedLine(float _x, float _y, float _h) override;
    void _drawHDottedLine(float _x, float _y, float _l) override;
    void _drawVLine(float _x, float _y, float _h) override;
    void _drawHLine(float _x, float _y, float _l) override;
    void _drawBMP(float _x, float _y, float _w, float _h, const unsigned char *_bitMap) override;
    void _drawBox(float _x, float _y, float _w, float _h) override;
    void _drawRBox(float _x, float _y, float _w, float _h, float _r) override;
    void _drawFrame(float _x, float _y, float _w, float _h) override;
    void _drawRFrame(float _x, float _y, float _w, float _h, float _r) override;
    void _screenOn() override;
    void _screenOff() override;

    // ---------- system timers ----------
    void _delay(unsigned long _mill) override;
    unsigned long _millis() override;
    unsigned long _getTick() override;
    unsigned long _getRandomSeed() override;

    // ---------- buzzer (stub) ----------
    void _beep(float _freq) override {}
    void _beepStop() override {}
    void _setBeepVol(unsigned char _vol) override {}

    // ---------- keys ----------
    bool _getKey(key::KEY_INDEX _keyIndex) override;

    // ---------- config ----------
    void _updateConfig() override {}
};
