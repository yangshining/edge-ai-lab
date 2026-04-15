/**
 * @file   selector.h
 * @brief  Selector class representing the animated highlight box in List and Tile menus.
 * @author Fir
 * @date   2024-04-14
 */

#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_ELEMENT_SELECTOR_SELECTOR_H_
#define ASTRA_CORE_SRC_ASTRA_UI_ELEMENT_SELECTOR_SELECTOR_H_
#include "../menu/menu.h"

namespace astra {

/** @brief Animated selection indicator that tracks the focused menu entry. */
class Selector : public Item {
private:
  Menu *menu; // Pointer to the menu this selector is bound to.

public:
  float x, xTrg;   ///< Current and target X position.
  float y, yTrg;   ///< Current and target Y position.

  float w, wTrg;   ///< Current and target width  (List layout only).
  float h, hTrg;   ///< Current and target height (List layout only).

  float yText, yTextTrg; ///< Current and target Y position of the tile title label.

  /**
   * @brief Default constructor.
   * @note  In Tile layout the selector box encompasses the entire tile icon; list layout uses a narrower highlight bar.
   *        The selector also tracks the tile title label position, which enables the title entry animation.
   */
  Selector() = default;

  /**
   * @brief  Return the current selector position as a two-element vector.
   * @return {x, y} current position.
   */
  std::vector<float> getPosition();

  /**
   * @brief  Snap the selector position to match the currently focused item.
   */
  void setPosition();

  /**
   * @brief  Move the selector to the item at the given index.
   * @param  _index  Zero-based index of the target item.
   */
  void go(unsigned char _index);

  /**
   * @brief  Move the selector to the next item in the menu.
   */
  void goNext();

  /**
   * @brief  Move the selector to the previous item in the menu.
   * @note   Method name is a known typo for goPrevious.
   */
  void goPreview();

  /**
   * @brief  Bind a Menu instance to this selector in preparation for rendering.
   * @param  _menu  Pointer to the Menu to inject.
   * @return True on success, false if the pointer is invalid.
   */
  bool inject(Menu *_menu);

  /**
   * @brief  Release the bound Menu instance.
   * @return True on success, false if no menu was bound.
   */
  bool destroy();

  /**
   * @brief  Draw the selector onto the canvas at the given camera offset.
   * @param  _camera  Two-element vector {camX, camY} representing the current viewport offset.
   */
  void render(std::vector<float> _camera);
};
}
#endif //ASTRA_CORE_SRC_ASTRA_UI_ELEMENT_SELECTOR_SELECTOR_H_
