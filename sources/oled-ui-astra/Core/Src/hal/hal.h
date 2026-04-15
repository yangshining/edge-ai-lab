/**
 * @file   hal.h
 * @brief  Abstract hardware abstraction layer interface for the Astra UI framework.
 * @author Fir
 * @date   2024-02-08
 */
#pragma once
#ifndef ASTRA_CORE_SRC_HAL_HAL_H_
#define ASTRA_CORE_SRC_HAL_HAL_H_

#include <string>
#include <utility>
#include <array>
#include <vector>
#include "../astra/config/config.h"

namespace oled {

}

/** @brief Key input types, filters, actions, and indices. */
namespace key {
typedef enum keyFilter {
  CHECKING = 0,
  KEY_0_CONFIRM,
  KEY_1_CONFIRM,
  RELEASED,
} KEY_FILTER;

typedef enum keyAction {
  INVALID = 0,
  CLICK,
  PRESS,
} KEY_ACTION;

typedef enum KeyType {
  KEY_NOT_PRESSED = 0,
  KEY_PRESSED,
} KEY_TYPE;

typedef enum keyIndex {
  KEY_0 = 0,
  KEY_1,
  KEY_NUM,
} KEY_INDEX;
}

namespace buzzer {

}

namespace led {

}

/** @brief System-level configuration and helpers. */
namespace sys {
/** @brief Holds screen dimensions and brightness settings. */
struct config {
  unsigned char screenWeight = 128; ///< Screen width in pixels.
  unsigned char screenHeight = 64;  ///< Screen height in pixels.
  float screenBright = 255;         ///< Screen brightness (0–255).
};

/**
 * @brief  Returns the singleton system configuration instance.
 * @return Reference to the static sys::config object.
 */
static config &getSystemConfig() {
  static config sysConfig;
  return sysConfig;
}
}

/**
 * @brief  Abstract hardware abstraction layer.
 *
 * Provides a static singleton interface to all hardware operations
 * (display, timers, buzzer, keys). Concrete boards subclass HAL and
 * are registered via HAL::inject().
 */
class HAL {
private:
  static HAL *hal; // Singleton instance pointer.

public:
  /**
   * @brief  Returns the active HAL instance.
   * @return Pointer to the current HAL singleton.
   */
  static HAL *get();

  /**
   * @brief  Checks whether a HAL instance has been injected.
   * @return true if a HAL instance exists, false otherwise.
   */
  static bool check();

  /**
   * @brief  Injects a concrete HAL implementation and runs its init().
   * @param  _hal  Pointer to the HAL subclass instance to register.
   * @return True if injection succeeded (non-null _hal was provided).
   */
  static bool inject(HAL *_hal);

  /**
   * @brief  Destroys the current HAL instance and resets the singleton.
   */
  static void destroy();

  virtual ~HAL() = default;

  /**
   * @brief  Returns a string identifying the concrete HAL type.
   * @return Human-readable type name.
   */
  virtual std::string type() { return "Base"; }

  /**
   * @brief  Initialises the hardware. Called once by inject().
   */
  virtual void init() {}

protected:
  sys::config config; // Cached system configuration for this HAL instance.

  // --- Canvas / display ---
public:
  /**
   * @brief  Returns a pointer to the raw display canvas buffer.
   * @return Void pointer to the frame buffer, or nullptr if unsupported.
   */
  static void *getCanvasBuffer() { return get()->_getCanvasBuffer(); }

  /** @brief Returns a pointer to the canvas frame buffer. */
  virtual void *_getCanvasBuffer() { return nullptr; }

  /**
   * @brief  Returns the canvas height expressed in 8-pixel tiles.
   * @return Tile height of the display buffer.
   */
  static unsigned char getBufferTileHeight() { return get()->_getBufferTileHeight(); }

  /** @brief Returns the canvas height in 8-pixel tiles. */
  virtual unsigned char _getBufferTileHeight() { return 0; }

  /**
   * @brief  Returns the canvas width expressed in 8-pixel tiles.
   * @return Tile width of the display buffer.
   */
  static unsigned char getBufferTileWidth() { return get()->_getBufferTileWidth(); }

  /** @brief Returns the canvas width in 8-pixel tiles. */
  virtual unsigned char _getBufferTileWidth() { return 0; }

  /**
   * @brief  Flushes the canvas buffer to the physical display.
   */
  static void canvasUpdate() { get()->_canvasUpdate(); }

  /** @brief Flushes the canvas buffer to the display. */
  virtual void _canvasUpdate() {}

  /**
   * @brief  Clears the canvas buffer.
   */
  static void canvasClear() { get()->_canvasClear(); }

  /** @brief Clears the canvas buffer. */
  virtual void _canvasClear() {}

  /**
   * @brief  Sets the active font for subsequent text drawing calls.
   * @param  _font  Pointer to the font data array.
   */
  static void setFont(const unsigned char *_font) { get()->_setFont(_font); }

  /** @brief Sets the active font for subsequent text drawing calls. */
  virtual void _setFont(const unsigned char *_font) {}

  /**
   * @brief  Returns the pixel width of a text string in the current font.
   * @param  _text  The string to measure.
   * @return Width in pixels.
   */
  static unsigned char getFontWidth(std::string &_text) { return get()->_getFontWidth(_text); }

  /** @brief Returns the pixel width of a text string in the current font. */
  virtual unsigned char _getFontWidth(std::string &_text) { return 0; }

  /**
   * @brief  Returns the pixel height of the current font.
   * @return Height in pixels.
   */
  static unsigned char getFontHeight() { return get()->_getFontHeight(); }

  /** @brief Returns the pixel height of the current font. */
  virtual unsigned char _getFontHeight() { return 0; }

  /**
   * @brief  Sets the drawing mode (e.g. XOR, solid fill).
   * @param  _type  Draw type identifier.
   */
  static void setDrawType(unsigned char _type) { get()->_setDrawType(_type); }

  /** @brief Sets the drawing mode (e.g. XOR, solid fill). */
  virtual void _setDrawType(unsigned char _type) {}

  /**
   * @brief  Draws a single pixel.
   * @param  _x  X coordinate.
   * @param  _y  Y coordinate.
   */
  static void drawPixel(float _x, float _y) { get()->_drawPixel(_x, _y); }

  /** @brief Draws a single pixel. */
  virtual void _drawPixel(float _x, float _y) {}

  /**
   * @brief  Draws an ASCII/English string.
   *
   * (_x, _y) is the coordinate of the bottom-left corner of the first glyph.
   *
   * @param  _x     X coordinate of the bottom-left corner.
   * @param  _y     Y coordinate of the bottom-left corner.
   * @param  _text  String to render.
   */
  static void drawEnglish(float _x, float _y, const std::string &_text) { get()->_drawEnglish(_x, _y, _text); }

  /** @brief Draws an ASCII/English string at the given coordinates. */
  virtual void _drawEnglish(float _x, float _y, const std::string &_text) {}

  /**
   * @brief  Draws a UTF-8 Chinese string.
   *
   * (_x, _y) is the coordinate of the bottom-left corner of the first glyph.
   *
   * @param  _x     X coordinate of the bottom-left corner.
   * @param  _y     Y coordinate of the bottom-left corner.
   * @param  _text  String to render.
   */
  static void drawChinese(float _x, float _y, const std::string &_text) { get()->_drawChinese(_x, _y, _text); }

  /** @brief Draws a UTF-8 Chinese string at the given coordinates. */
  virtual void _drawChinese(float _x, float _y, const std::string &_text) {}

  /**
   * @brief  Draws a vertical dotted line.
   * @param  _x  X coordinate of the line.
   * @param  _y  Y coordinate of the top of the line.
   * @param  _h  Height in pixels.
   */
  static void drawVDottedLine(float _x, float _y, float _h) { get()->_drawVDottedLine(_x, _y, _h); }

  /** @brief Draws a vertical dotted line. */
  virtual void _drawVDottedLine(float _x, float _y, float _h) {}

  /**
   * @brief  Draws a horizontal dotted line.
   * @param  _x  X coordinate of the left end.
   * @param  _y  Y coordinate of the line.
   * @param  _l  Length in pixels.
   */
  static void drawHDottedLine(float _x, float _y, float _l) { get()->_drawHDottedLine(_x, _y, _l); }

  /** @brief Draws a horizontal dotted line. */
  virtual void _drawHDottedLine(float _x, float _y, float _l) {}

  /**
   * @brief  Draws a solid vertical line.
   * @param  _x  X coordinate.
   * @param  _y  Y coordinate of the top.
   * @param  _h  Height in pixels.
   */
  static void drawVLine(float _x, float _y, float _h) { get()->_drawVLine(_x, _y, _h); }

  /** @brief Draws a solid vertical line. */
  virtual void _drawVLine(float _x, float _y, float _h) {}

  /**
   * @brief  Draws a solid horizontal line.
   * @param  _x  X coordinate of the left end.
   * @param  _y  Y coordinate.
   * @param  _l  Length in pixels.
   */
  static void drawHLine(float _x, float _y, float _l) { get()->_drawHLine(_x, _y, _l); }

  /** @brief Draws a solid horizontal line. */
  virtual void _drawHLine(float _x, float _y, float _l) {}

  /**
   * @brief  Draws a bitmap image.
   * @param  _x       X coordinate of the top-left corner.
   * @param  _y       Y coordinate of the top-left corner.
   * @param  _w       Width in pixels.
   * @param  _h       Height in pixels.
   * @param  _bitMap  Pointer to the bitmap data.
   */
  static void drawBMP(float _x, float _y, float _w, float _h, const unsigned char *_bitMap) {
    get()->_drawBMP(_x,
                    _y,
                    _w,
                    _h,
                    _bitMap);
  }

  /** @brief Draws a bitmap image at the given position and size. */
  virtual void _drawBMP(float _x, float _y, float _w, float _h, const unsigned char *_bitMap) {}

  /**
   * @brief  Draws a filled rectangle.
   * @param  _x  X coordinate of the top-left corner.
   * @param  _y  Y coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   */
  static void drawBox(float _x, float _y, float _w, float _h) { get()->_drawBox(_x, _y, _w, _h); }

  /** @brief Draws a filled rectangle. */
  virtual void _drawBox(float _x, float _y, float _w, float _h) {}

  /**
   * @brief  Draws a filled rounded rectangle.
   * @param  _x  X coordinate of the top-left corner.
   * @param  _y  Y coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   * @param  _r  Corner radius in pixels.
   */
  static void drawRBox(float _x, float _y, float _w, float _h, float _r) {
    get()->_drawRBox(_x,
                     _y,
                     _w,
                     _h,
                     _r);
  }

  /** @brief Draws a filled rounded rectangle. */
  virtual void _drawRBox(float _x, float _y, float _w, float _h, float _r) {}

  /**
   * @brief  Draws an unfilled rectangle (frame only).
   * @param  _x  X coordinate of the top-left corner.
   * @param  _y  Y coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   */
  static void drawFrame(float _x, float _y, float _w, float _h) { get()->_drawFrame(_x, _y, _w, _h); }

  /** @brief Draws an unfilled rectangle (frame only). */
  virtual void _drawFrame(float _x, float _y, float _w, float _h) {}

  /**
   * @brief  Draws an unfilled rounded rectangle (frame only).
   * @param  _x  X coordinate of the top-left corner.
   * @param  _y  Y coordinate of the top-left corner.
   * @param  _w  Width in pixels.
   * @param  _h  Height in pixels.
   * @param  _r  Corner radius in pixels.
   */
  static void drawRFrame(float _x, float _y, float _w, float _h, float _r) {
    get()->_drawRFrame(_x,
                       _y,
                       _w,
                       _h,
                       _r);
  }

  /** @brief Draws an unfilled rounded rectangle (frame only). */
  virtual void _drawRFrame(float _x, float _y, float _w, float _h, float _r) {}

  /**
   * @brief  Prints a diagnostic message to the debug output.
   * @param  _msg  Message string to print.
   */
  static void printInfo(std::string _msg) { get()->_printInfo(std::move(_msg)); }

  /** @brief Prints a diagnostic message to the debug output. */
  virtual void _printInfo(std::string _msg);

  // --- System timers ---
public:
  /**
   * @brief  Blocks execution for the specified number of milliseconds.
   * @param  _mill  Delay duration in milliseconds.
   */
  static void delay(unsigned long _mill) { get()->_delay(_mill); }

  /** @brief Blocks execution for the specified number of milliseconds. */
  virtual void _delay(unsigned long _mill) {}

  /**
   * @brief  Returns the number of milliseconds since startup.
   * @return Elapsed time in milliseconds.
   */
  static unsigned long millis() { return get()->_millis(); }

  /** @brief Returns the number of milliseconds since startup. */
  virtual unsigned long _millis() { return 0; }

  /**
   * @brief  Returns the raw hardware tick counter.
   * @return Current tick value.
   */
  static unsigned long getTick() { return get()->_getTick(); }

  /** @brief Returns the raw hardware tick counter. */
  virtual unsigned long _getTick() { return 0; }

  /**
   * @brief  Returns a seed value for random number generation (optional).
   * @return Random seed, or 0 if not supported by the platform.
   */
  static unsigned long getRandomSeed() { return get()->_getRandomSeed(); }

  /** @brief Returns a seed value for random number generation; returns 0 if not supported. */
  virtual unsigned long _getRandomSeed() { return 0; }

  // --- Buzzer ---
public:
  /**
   * @brief  Starts the buzzer at the specified frequency.
   * @param  _freq  Frequency in Hz.
   */
  static void beep(float _freq) { get()->_beep(_freq); }

  /** @brief Starts the buzzer at the specified frequency. */
  virtual void _beep(float _freq) {}

  /**
   * @brief  Stops the buzzer.
   */
  static void beepStop() { get()->_beepStop(); }

  /** @brief Stops the buzzer. */
  virtual void _beepStop() {}

  /**
   * @brief  Sets the buzzer volume.
   * @param  _vol  Volume level (0–255).
   */
  static void setBeepVol(unsigned char _vol) { get()->_setBeepVol(_vol); }

  /** @brief Sets the buzzer volume. */
  virtual void _setBeepVol(unsigned char _vol) {}

  /**
   * @brief  Turns the screen on.
   */
  static void screenOn() { get()->_screenOn(); }

  /** @brief Turns the screen on. */
  virtual void _screenOn() {}

  /**
   * @brief  Turns the screen off.
   */
  static void screenOff() { get()->_screenOff(); }

  /** @brief Turns the screen off. */
  virtual void _screenOff() {}

  // --- Key input ---
public:
  key::KEY_ACTION key[key::KEY_NUM] = {static_cast<key::keyAction>(0)}; ///< Per-key action state array.
  key::KEY_TYPE keyFlag { static_cast<key::KEY_TYPE>(0) };              ///< Global key-pressed flag.

public:
  /**
   * @brief  Returns the current action state of a single key.
   * @param  _keyIndex  Index of the key to query.
   * @return True for CLICK or PRESS actions, false for INVALID (no input).
   */
  static bool getKey(key::KEY_INDEX _keyIndex) { return get()->_getKey(_keyIndex); }

  /** @brief Returns true for CLICK or PRESS actions, false for INVALID (no input). */
  virtual bool _getKey(key::KEY_INDEX _keyIndex) { return false; }

  /**
   * @brief  Returns true if any key is currently active.
   * @return true if at least one key action is non-INVALID.
   */
  static bool getAnyKey() { return get()->_getAnyKey(); }

  /** @brief Returns true if at least one key action is non-INVALID. */
  virtual bool _getAnyKey();

  /**
   * @brief  Returns a pointer to the full key action map array.
   * @return Pointer to the key::KEY_ACTION array of length key::KEY_NUM.
   */
  static key::KEY_ACTION *getKeyMap() { return get()->key; }

  /**
   * @brief  Returns a pointer to the global key-pressed flag.
   * @return Pointer to the key::KEY_TYPE flag member.
   */
  static key::KEY_TYPE *getKeyFlag() { return &get()->keyFlag; }

public:
  /**
   * @brief  Scans all hardware keys and updates the key action map.
   */
  static void keyScan() { get()->_keyScan(); }

  /** @brief Scans all hardware keys and updates the key action map. */
  virtual void _keyScan();

  /**
   * @brief  Runs a blocking key test routine (debug utility).
   */
  static void keyTest() { return get()->_keyTest(); }

  /** @brief Runs a blocking key test routine (debug utility). */
  virtual void _keyTest();

  // --- System configuration ---
public:
  /**
   * @brief  Returns a reference to the system configuration held by this HAL instance.
   * @return Reference to sys::config.
   */
  static sys::config &getSystemConfig() { return get()->config; }

  /**
   * @brief  Replaces the system configuration held by this HAL instance.
   * @param  _cfg  New configuration to apply.
   */
  static void setSystemConfig(sys::config _cfg) { get()->config = _cfg; }

  /**
   * @brief  Propagates the current system configuration to the hardware.
   */
  static void updateConfig() { get()->_updateConfig(); }

  /** @brief Propagates the current system configuration to the hardware. */
  virtual void _updateConfig() {}
};

#endif //ASTRA_CORE_SRC_HAL_HAL_H_
