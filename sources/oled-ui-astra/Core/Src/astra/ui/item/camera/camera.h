/**
 * @file   camera.h
 * @brief  Camera class that provides a scrollable viewport over the menu tree.
 * @author Fir
 * @date   2024-04-14
 */

#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_ELEMENT_CAMERA_CAMERA_H_
#define ASTRA_CORE_SRC_ASTRA_UI_ELEMENT_CAMERA_CAMERA_H_

#include "../selector/selector.h"

namespace astra {

// The camera moves with the selected item; the element's absolute coordinates
// never change — the camera offset is added only at render time.
// Static foreground elements (dotted lines, progress bars, arrows, title text)
// are rendered independently and are not affected by the camera.
//   Tile layout  — camera moves horizontally; foreground: dotted line, title,
//                  arrow, and button icon.
//   List layout  — camera moves vertically;   foreground: progress bar.

/** @brief Virtual viewport that pans over a Menu to implement smooth scrolling. */
class Camera : public Item {
private:
  float xInit; ///< Initial X position (saved camera origin).
  float yInit; ///< Initial Y position (saved camera origin).

public:
  float x, y;       ///< Current camera position.
  float xTrg, yTrg; ///< Target camera position for animated movement.

  /**
   * @brief  Construct a Camera at position (0, 0).
   */
  Camera();

  /**
   * @brief  Construct a Camera at the given position.
   * @param  _x  Initial X position.
   * @param  _y  Initial Y position.
   */
  Camera(float _x, float _y);

  /**
   * @brief  Test whether a point lies outside the current viewport.
   * @param  _x  X coordinate to test.
   * @param  _y  Y coordinate to test.
   * @return Non-zero if the point is out of view, zero otherwise.
   */
  unsigned char outOfView(float _x, float _y);

  /**
   * @brief  Convenience overload accepting a vector of coordinates.
   * @param  _posVec  Vector of coordinate values to check.
   * @return True if any coordinate is outside the viewport.
   */
  unsigned char outOfView(std::vector<float> _pos) { return outOfView(_pos[0], _pos[1]); }

  /**
   * @brief  Return the current camera position as a two-element vector.
   * @return {x, y} current position.
   */
  [[nodiscard]] std::vector<float> getPosition();

  /**
   * @brief  Return the camera target position as a two-element vector.
   * @return {xTrg, yTrg} target position.
   */
  [[nodiscard]] std::vector<float> getPositionTrg();

  /**
   * @brief  Initialise the camera layout according to menu type.
   * @param  _type  Menu type string ("list" or "tile").
   */
  void init(const std::string &_type);

  // The Launcher creates Selector and Camera instances, then injects them into
  // the menu renderer. Calling the movement methods below from the Launcher
  // drives the viewport. All rendered element coordinates remain fixed in
  // absolute space; only the camera position changes.

  /**
   * @brief  Set the animated target to the given position (camera will glide there).
   * @param  _x  Target X coordinate.
   * @param  _y  Target Y coordinate.
   */
  void go(float _x, float _y);

  /**
   * @brief  Set the animated target to the given position vector.
   * @param  _pos  Two-element vector {x, y}.
   */
  void go(const std::vector<float> &_pos);

  /**
   * @brief  Teleport the camera to the given position with no animation.
   * @param  _x  Target X coordinate.
   * @param  _y  Target Y coordinate.
   */
  void goDirect(float _x, float _y);

  /**
   * @brief  Offset the animated target by the given delta.
   * @param  _x  X delta.
   * @param  _y  Y delta.
   */
  void move(float _x, float _y);

  /**
   * @brief  Offset the current position immediately with no animation.
   * @param  _x  X delta.
   * @param  _y  Y delta.
   */
  void moveDirect(float _x, float _y);

  /**
   * @brief  Pan the camera to keep the selected list item visible (rolling mode).
   * @param  _menu  Pointer to the active List menu.
   */
  void goToListItemRolling(List *_menu);

  /**
   * @brief  Pan the camera to centre on the tile item at the given index.
   * @param  _index  Zero-based tile index.
   */
  void goToTileItem(unsigned char _index);

  /**
   * @brief  Animate the camera back to its initial position.
   */
  void reset();

  /**
   * @brief  Teleport the camera back to its initial position with no animation.
   */
  void resetDirect();

  /**
   * @brief  Update camera position each frame based on the active menu and selector state.
   * @param  _menu      Pointer to the currently displayed Menu.
   * @param  _selector  Pointer to the active Selector.
   */
  void update(Menu *_menu, Selector *_selector);

  /**
   * @brief  Render any camera-relative foreground elements onto the canvas.
   */
  void render();
};

}

#endif //ASTRA_CORE_SRC_ASTRA_UI_ELEMENT_CAMERA_CAMERA_H_
