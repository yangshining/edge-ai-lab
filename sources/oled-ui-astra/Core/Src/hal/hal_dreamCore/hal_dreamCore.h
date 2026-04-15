/**
 * @file   hal_dreamCore.h
 * @brief  Concrete HAL implementation for the STM32F103-based DreamCore board.
 * @author Fir
 * @date   2024-02-11
 */

#pragma once
#ifndef ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_HAL_DREAMCORE_H_
#define ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_HAL_DREAMCORE_H_
#include "../hal.h"
#include "components/oled/graph_lib/u8g2/u8g2.h"

/** @brief Concrete HAL implementation targeting the STM32F103 DreamCore board. */
class HALDreamCore : public HAL {
private:
  // Low-level STM32 peripheral initializers called from init().
  void _stm32_hal_init();
  void _sys_clock_init();
  void _gpio_init();
  void _dma_init();
  void _timer_init();
  void _spi_init();
  void _adc_init();

  // Board-level component initializers called from init().
  void _ssd1306_init();
  void _key_init();
  void _buzzer_init();
  void _u8g2_init();

public:
  /** @brief Default constructor. */
  HALDreamCore() = default;

protected:
  u8g2_t canvasBuffer {}; ///< u8g2 canvas state and frame buffer.

  /**
   * @brief  SPI byte-transfer callback required by u8x8.
   * @param  _u8x8    Pointer to the u8x8 instance.
   * @param  _msg     Message/command identifier.
   * @param  _argInt  Integer argument for the message.
   * @param  _argPtr  Pointer argument for the message.
   * @return Non-zero on success, zero on failure.
   */
  static unsigned char _u8x8_byte_hw_spi_callback(u8x8_t* _u8x8, unsigned char _msg, unsigned char _argInt, void* _argPtr);

  /**
   * @brief  GPIO and delay callback stub; all parameters are unused by this implementation.
   * @param  _u8x8    Not used.
   * @param  _msg     Not used.
   * @param  _argInt  Not used.
   * @param  _argPtr  Not used.
   * @return Always returns 1 (success).
   */
  static unsigned char _u8x8_gpio_and_delay_callback(U8X8_UNUSED u8x8_t* _u8x8,
    U8X8_UNUSED unsigned char _msg,
    U8X8_UNUSED unsigned char _argInt,
    U8X8_UNUSED void* _argPtr);

public:
  /**
   * @brief  Initializes all STM32 peripherals and board components.
   * @note   Overrides HAL::init.
   */
  inline void init() override {
    _stm32_hal_init();
    _sys_clock_init();
    _gpio_init();
    _dma_init();
    _timer_init();
    _spi_init();
    _adc_init();

    _ssd1306_init();
    _key_init();
    _buzzer_init();
    _u8g2_init();
  }

protected:
  // Low-level SSD1306 communication helpers.
  void _ssd1306_transmit_cmd(unsigned char _cmd);
  void _ssd1306_transmit_data(unsigned char _data, unsigned char _mode);
  void _ssd1306_reset(bool _state);
  void _ssd1306_set_cursor(unsigned char _x, unsigned char _y);
  void _ssd1306_fill(unsigned char _data);

public:
  /**
   * @brief  Powers on the OLED display.
   * @note   Overrides HAL::_screenOn.
   */
  void _screenOn() override;

  /**
   * @brief  Powers off the OLED display.
   * @note   Overrides HAL::_screenOff.
   */
  void _screenOff() override;

public:
  /**
   * @brief  Returns a pointer to the u8g2 canvas buffer.
   * @return Void pointer to the internal canvas buffer.
   * @note   Overrides HAL::_getCanvasBuffer.
   */
  void* _getCanvasBuffer() override;

  /**
   * @brief  Returns the canvas tile height.
   * @return Tile height in 8-pixel units.
   * @note   Overrides HAL::_getBufferTileHeight.
   */
  unsigned char _getBufferTileHeight() override;

  /**
   * @brief  Returns the canvas tile width.
   * @return Tile width in 8-pixel units.
   * @note   Overrides HAL::_getBufferTileWidth.
   */
  unsigned char _getBufferTileWidth() override;

  /**
   * @brief  Flushes the canvas buffer to the OLED display.
   * @note   Overrides HAL::_canvasUpdate.
   */
  void _canvasUpdate() override;

  /**
   * @brief  Clears the canvas buffer.
   * @note   Overrides HAL::_canvasClear.
   */
  void _canvasClear() override;

  /**
   * @brief  Sets the active font for subsequent text drawing.
   * @param  _font  Pointer to a u8g2 font data array.
   * @note   Overrides HAL::_setFont.
   */
  void _setFont(const unsigned char * _font) override;

  /**
   * @brief  Returns the pixel width of a text string in the current font.
   * @param  _text  Reference to the string to measure.
   * @return Width in pixels.
   * @note   Overrides HAL::_getFontWidth.
   */
  unsigned char _getFontWidth(std::string& _text) override;

  /**
   * @brief  Returns the pixel height of the current font.
   * @return Height in pixels.
   * @note   Overrides HAL::_getFontHeight.
   */
  unsigned char _getFontHeight() override;

  /**
   * @brief  Sets the draw color/type (e.g., normal or XOR).
   * @param  _type  Draw type value passed to u8g2.
   * @note   Overrides HAL::_setDrawType.
   */
  void _setDrawType(unsigned char _type) override;

  /**
   * @brief  Draws a single pixel.
   * @param  _x  Horizontal coordinate.
   * @param  _y  Vertical coordinate.
   * @note   Overrides HAL::_drawPixel.
   */
  void _drawPixel(float _x, float _y) override;

  /**
   * @brief  Draws an ASCII/English string at the specified position.
   * @param  _x     Horizontal coordinate.
   * @param  _y     Vertical coordinate.
   * @param  _text  String to render.
   * @note   Overrides HAL::_drawEnglish.
   */
  void _drawEnglish(float _x, float _y, const std::string& _text) override;

  /**
   * @brief  Draws a UTF-8 Chinese string at the specified position.
   * @param  _x     Horizontal coordinate.
   * @param  _y     Vertical coordinate.
   * @param  _text  UTF-8 encoded string to render.
   * @note   Overrides HAL::_drawChinese.
   */
  void _drawChinese(float _x, float _y, const std::string& _text) override;

  /**
   * @brief  Draws a vertical dotted line.
   * @param  _x  Horizontal coordinate of the line.
   * @param  _y  Vertical start coordinate.
   * @param  _h  Length of the line in pixels.
   * @note   Overrides HAL::_drawVDottedLine.
   */
  void _drawVDottedLine(float _x, float _y, float _h) override;

  /**
   * @brief  Draws a horizontal dotted line.
   * @param  _x  Horizontal start coordinate.
   * @param  _y  Vertical coordinate of the line.
   * @param  _l  Length of the line in pixels.
   * @note   Overrides HAL::_drawHDottedLine.
   */
  void _drawHDottedLine(float _x, float _y, float _l) override;

  /**
   * @brief  Draws a solid vertical line.
   * @param  _x  Horizontal coordinate of the line.
   * @param  _y  Vertical start coordinate.
   * @param  _h  Length of the line in pixels.
   * @note   Overrides HAL::_drawVLine.
   */
  void _drawVLine(float _x, float _y, float _h) override;

  /**
   * @brief  Draws a solid horizontal line.
   * @param  _x  Horizontal start coordinate.
   * @param  _y  Vertical coordinate of the line.
   * @param  _l  Length of the line in pixels.
   * @note   Overrides HAL::_drawHLine.
   */
  void _drawHLine(float _x, float _y, float _l) override;

  /**
   * @brief  Draws a bitmap image.
   * @param  _x       Horizontal coordinate of the top-left corner.
   * @param  _y       Vertical coordinate of the top-left corner.
   * @param  _w       Width of the bitmap in pixels.
   * @param  _h       Height of the bitmap in pixels.
   * @param  _bitMap  Pointer to the bitmap data array.
   * @note   Overrides HAL::_drawBMP.
   */
  void _drawBMP(float _x, float _y, float _w, float _h, const unsigned char* _bitMap) override;

  /**
   * @brief  Draws a filled rectangle.
   * @param  _x  Horizontal coordinate of the top-left corner.
   * @param  _y  Vertical coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   * @note   Overrides HAL::_drawBox.
   */
  void _drawBox(float _x, float _y, float _w, float _h) override;

  /**
   * @brief  Draws a filled rounded rectangle.
   * @param  _x  Horizontal coordinate of the top-left corner.
   * @param  _y  Vertical coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   * @param  _r  Corner radius in pixels.
   * @note   Overrides HAL::_drawRBox.
   */
  void _drawRBox(float _x, float _y, float _w, float _h, float _r) override;

  /**
   * @brief  Draws a rectangle outline.
   * @param  _x  Horizontal coordinate of the top-left corner.
   * @param  _y  Vertical coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   * @note   Overrides HAL::_drawFrame.
   */
  void _drawFrame(float _x, float _y, float _w, float _h) override;

  /**
   * @brief  Draws a rounded rectangle outline.
   * @param  _x  Horizontal coordinate of the top-left corner.
   * @param  _y  Vertical coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   * @param  _r  Corner radius in pixels.
   * @note   Overrides HAL::_drawRFrame.
   */
  void _drawRFrame(float _x, float _y, float _w, float _h, float _r) override;

public:
  /**
   * @brief  Blocks execution for the specified number of milliseconds.
   * @param  _mill  Delay duration in milliseconds.
   * @note   Overrides HAL::_delay.
   */
  void _delay(unsigned long _mill) override;

  /**
   * @brief  Returns elapsed time in milliseconds since boot (alias for HAL_GetTick()).
   * @return Elapsed time in milliseconds.
   * @note   Overrides HAL::_millis.
   */
  unsigned long _millis() override;

  /**
   * @brief  Returns the raw SysTick-derived millisecond counter (equivalent to HAL_GetTick()).
   * @return Current tick count.
   * @note   Overrides HAL::_getTick.
   */
  unsigned long _getTick() override;

  /**
   * @brief  Returns a seed value suitable for random number generation.
   * @return Seed derived from a hardware source (e.g., ADC noise).
   * @note   Overrides HAL::_getRandomSeed.
   */
  unsigned long _getRandomSeed() override;

public:
  /**
   * @brief  Starts the buzzer at the specified frequency.
   * @param  _freq  Frequency in Hz.
   * @note   Overrides HAL::_beep.
   */
  void _beep(float _freq) override;

  /**
   * @brief  Stops the buzzer.
   * @note   Overrides HAL::_beepStop.
   */
  void _beepStop() override;

  /**
   * @brief  Sets the buzzer volume (PWM duty cycle).
   * @param  _vol  Volume level (0–255).
   * @note   Overrides HAL::_setBeepVol.
   */
  void _setBeepVol(unsigned char _vol) override;

public:
  /**
   * @brief  Returns the current state of the specified key.
   * @param  _keyIndex  Logical key index to query.
   * @return True if the key is pressed, false otherwise.
   * @note   Overrides HAL::_getKey.
   */
  bool _getKey(key::KEY_INDEX _keyIndex) override;

public:
  /**
   * @brief  Applies any pending configuration changes to the HAL.
   * @note   Overrides HAL::_updateConfig.
   */
  void _updateConfig() override;
};

#endif //ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_HAL_DREAMCORE_H_
