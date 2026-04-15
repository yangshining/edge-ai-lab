/**
 * @file   widget.h
 * @brief  Interactive widget base class and the three concrete types: CheckBox, PopUp, Slider.
 * @author Fir
 * @date   2024-01-21
 */
#pragma once
#ifndef ASTRA_WIDGET__H
#define ASTRA_WIDGET__H

#include "../item.h"

namespace astra {

/** @brief Abstract base for all interactive widgets that attach to list entries. */
class Widget : public Item {
public:
  void *parent{};        ///< Pointer to the owning menu entry (type-erased).
  unsigned char value{}; ///< Current value managed by this widget.

public:
  /**
   * @brief  Return a string identifying the widget type.
   * @return "base" for the base class; overridden in subclasses.
   */
  [[nodiscard]] virtual std::string getType() const { return "base"; }

public:
  Widget() = default;

public:
  /**
   * @brief  Prepare the widget for interaction; called when the parent menu opens.
   */
  virtual void init() {}

  /**
   * @brief  Tear down the widget; called when the parent menu closes.
   */
  virtual void deInit() {}
  // open/close transitions are driven by the Launcher, not by the widget itself.

public:
  /**
   * @brief  Render the small in-list indicator icon for this widget.
   * @param  _x       X coordinate of the host list entry.
   * @param  _y       Y coordinate of the host list entry.
   * @param  _camera  Current camera state vector.
   */
  virtual void renderIndicator(float _x, float _y, const std::vector<float> &_camera) {}

public:
  /**
   * @brief  Render the full widget overlay.
   * @param  _camera  Current camera state vector.
   */
  virtual void render(const std::vector<float> &_camera) {}
};

/** @brief Boolean toggle widget displayed as a check box in a list entry. */
class CheckBox : public Widget {
public:
  /**
   * @brief  Return the widget type identifier.
   * @return "CheckBox".
   * @note   Overrides Widget::getType.
   */
  [[nodiscard]] std::string getType() const override { return "CheckBox"; }

private:
  bool isCheck; // Current checked state.

public:
  /**
   * @brief  Construct a CheckBox bound to an external boolean flag.
   * @param  _value  Reference to the boolean variable this widget controls.
   */
  explicit CheckBox(bool &_value);

public:
  /**
   * @brief  Set the check box to the checked state.
   * @return True if the state changed.
   */
  bool check();

  /**
   * @brief  Set the check box to the unchecked state.
   * @return True if the state changed.
   */
  bool uncheck();

  /**
   * @brief  Toggle the check box between checked and unchecked.
   * @return The new checked state (true = checked).
   */
  bool toggle();

public:
  /**
   * @brief  Initialise the check box; syncs internal state from the bound variable.
   * @note   Overrides Widget::init.
   */
  void init() override;

  /**
   * @brief  Tear down the check box.
   * @note   Overrides Widget::deInit.
   */
  void deInit() override;

public:
  /**
   * @brief  Render the small check-box indicator inside the list row.
   * @param  _x       X coordinate of the host list entry.
   * @param  _y       Y coordinate of the host list entry.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Widget::renderIndicator.
   */
  void renderIndicator(float _x, float _y, const std::vector<float> &_camera) override;

public:
  /**
   * @brief  Render the full check-box overlay.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Widget::render.
   */
  void render(const std::vector<float> &_camera) override;
};

/** @brief Pop-up option selector widget that slides in from a configurable edge. */
class PopUp : public Widget {
public:
  /**
   * @brief  Return the widget type identifier.
   * @return "PopUp".
   * @note   Overrides Widget::getType.
   */
  [[nodiscard]] std::string getType() const override { return "PopUp"; }

public:
  /** @brief Animated (current, target) screen position of the pop-up panel. */
  typedef struct Position {
    float x, xTrg; ///< Current and target x coordinate.
    float y, yTrg; ///< Current and target y coordinate.
  } Position;

  Position position{}; ///< Pop-up panel animation state.

private:
  std::string title;                 // Heading shown inside the pop-up panel.
  std::vector<std::string> options;  // Selectable option strings.
  unsigned char direction;           // Slide-in edge: 0 = left, 1 = top, 2 = right, 3 = bottom.
  unsigned char boundary;            // Index of the last option; equals options.size() - 1.

public:
  /**
   * @brief  Construct a PopUp widget.
   * @param  _direction  Edge from which the panel slides in (0=left, 1=top, 2=right, 3=bottom).
   * @param  _title      Heading text displayed inside the panel.
   * @param  _options    List of option strings to choose from.
   * @param  _value      Reference to the variable that receives the selected index.
   */
  PopUp(unsigned char _direction,
        const std::string &_title,
        const std::vector<std::string> &_options,
        unsigned char &_value);

public:
  /**
   * @brief  Advance selection to the next option, wrapping at the end.
   */
  void selectNext();

  /**
   * @brief  Moves the selection to the previous option, wrapping at the beginning.
   * @note   Method name is a known typo for selectPrevious.
   */
  void selectPreview();

  /**
   * @brief  Select the option at the given index directly.
   * @param  _index  Zero-based index of the option to select.
   * @return True if the index was valid and the selection changed.
   */
  bool select(unsigned char _index);

public:
  /**
   * @brief  Initialise the pop-up widget and reset its animated position.
   * @note   Overrides Widget::init.
   */
  void init() override;

  /**
   * @brief  Tear down the pop-up widget.
   * @note   Overrides Widget::deInit.
   */
  void deInit() override;

public:
  /**
   * @brief  Render the small pop-up indicator inside the list row.
   * @param  _x       X coordinate of the host list entry.
   * @param  _y       Y coordinate of the host list entry.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Widget::renderIndicator.
   */
  void renderIndicator(float _x, float _y, const std::vector<float> &_camera) override;

public:
  /**
   * @brief  Render the full pop-up panel overlay.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Widget::render.
   */
  void render(const std::vector<float> &_camera) override;
};

/** @brief Horizontal drag slider widget for selecting a value within a bounded range. */
class Slider : public Widget {
public:
  /**
   * @brief  Return the widget type identifier.
   * @return "Slider".
   * @note   Overrides Widget::getType.
   */
  [[nodiscard]] std::string getType() const override { return "Slider"; }

public:
  /** @brief Animated (current, target) position and length of the slider track. */
  typedef struct Position {
    float x, xTrg; ///< Current and target x coordinate of the slider panel.
    float y, yTrg; ///< Current and target y coordinate of the slider panel.
    float l, lTrg; ///< Current and target filled length of the slider track.
  } Position;

  Position position{}; ///< Slider panel and track animation state.

private:
  std::string title;          // Label displayed above the slider track.
  unsigned char maxLength;    // Pixel width of the full slider track.
  unsigned char min;          // Minimum raw value.
  unsigned char max;          // Maximum raw value.
  unsigned char step;         // Increment/decrement step size.

  bool valueOverflow;         // True when the last add() or sub() call hit the min or max boundary.

  unsigned char lengthIndicator; // Current filled-track length in pixels, derived from value.

public:
  /**
   * @brief  Construct a Slider widget.
   * @param  _title  Label text shown above the track.
   * @param  _min    Minimum selectable value.
   * @param  _max    Maximum selectable value.
   * @param  _step   Step size for each add/sub call.
   * @param  _value  Reference to the variable that receives the current value.
   */
  Slider(const std::string &_title,
         unsigned char _min,
         unsigned char _max,
         unsigned char _step,
         unsigned char &_value);

public:
  /**
   * @brief  Increment the slider value by one step, clamping at max.
   * @return The new value after incrementing.
   */
  unsigned char add();

  /**
   * @brief  Decrement the slider value by one step, clamping at min.
   * @return The new value after decrementing.
   */
  unsigned char sub();

public:
  /**
   * @brief  Initialise the slider and sync the track length from the current value.
   * @note   Overrides Widget::init.
   */
  void init() override;

  /**
   * @brief  Tear down the slider widget.
   * @note   Overrides Widget::deInit.
   */
  void deInit() override;

public:
  /**
   * @brief  Render the small slider indicator inside the list row.
   * @param  _x       X coordinate of the host list entry.
   * @param  _y       Y coordinate of the host list entry.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Widget::renderIndicator.
   */
  void renderIndicator(float _x, float _y, const std::vector<float> &_camera) override;

public:
  /**
   * @brief  Render the full slider panel overlay.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Widget::render.
   */
  void render(const std::vector<float> &_camera) override;
};
}

#endif //ASTRA_WIDGET__H
