/**
 * @file   launcher.h
 * @brief  Frame-loop driver that owns the camera, selector, and navigation state.
 * @author Fir
 * @date   2024-02-02
 */
#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_
#define ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_

#include "item/menu/menu.h"
#include "item/selector/selector.h"
#include "item/camera/camera.h"

namespace astra {

/** @brief Drives the UI frame loop and manages navigation between menus. */
class Launcher {
private:
  Menu* currentMenu;              ///< The menu currently being displayed.
  Widget* currentWidget = nullptr; ///< The widget currently being interacted with, or nullptr.
  Selector* selector;             ///< Animated highlight box tracking the selected item.
  Camera* camera;                 ///< Virtual viewport offset applied to all rendering.

  uint64_t time; ///< Timestamp used for timed info popups.

public:
  /**
   * @brief  Display a transient info string for a fixed duration.
   * @param  _info  Text to display.
   * @param  _time  Display duration in milliseconds.
   */
  void popInfo(std::string _info, uint16_t _time);

  /**
   * @brief  Initialise the launcher with the root menu page.
   * @param  _rootPage  Pointer to the top-level menu node.
   */
  void init(Menu* _rootPage);

  /**
   * @brief  Open (enter) the currently selected child menu or activate the current widget.
   * @return True if the action was handled, false otherwise.
   */
  bool open();

  /**
   * @brief  Close (leave) the current menu and return to its parent.
   * @return True if the action was handled, false otherwise.
   */
  bool close();

  /**
   * @brief  Execute one frame: process input, advance animations, and render.
   */
  void update();

  /**
   * @brief  Return a pointer to the camera instance.
   * @return Pointer to the Camera owned by this launcher.
   */
  Camera* getCamera() { return camera; }

  /**
   * @brief  Return a pointer to the selector instance.
   * @return Pointer to the Selector owned by this launcher.
   */
  Selector* getSelector() { return selector; }
};
}

#endif //ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_
