/**
 * @file   menu.h
 * @brief  Menu node base class and the two concrete layout types: List and Tile.
 * @author Fir
 * @date   2024-01-21
 */

#pragma once
#ifndef ASTRA_ASTRA__H
#define ASTRA_ASTRA__H

#include "cstdint"
#include "string"
#include <vector>
#include "../item.h"
#include "../widget/widget.h"

namespace astra {

/** @brief Base node for the page tree; holds children, position, and lifecycle state. */
class Menu : public Item {
public:
  /**
   * @brief  Return a string identifying the menu layout type.
   * @return "Base" for the base class; overridden in subclasses.
   */
  [[nodiscard]] virtual std::string getType() const { return "Base"; }

public:
  std::vector<float> cameraPosMemory = {}; ///< Saved camera position for this page.

  /**
   * @brief  Save the current camera position so it can be restored on re-entry.
   * @param  _camera  Camera state vector to remember.
   */
  void rememberCameraPos(const std::vector<float> &_camera) {
    cameraPosMemory = _camera;
    cameraPosMemoryFlag = true;
  }

  /**
   * @brief  Return the previously saved camera position.
   * @return Saved camera state vector.
   */
  [[nodiscard]] std::vector<float> getCameraMemoryPos() const { return cameraPosMemory; }

  /**
   * @brief  Reset the saved camera position to the origin.
   */
  void resetCameraMemoryPos() { cameraPosMemory = {0, 0}; }

  bool cameraPosMemoryFlag = false; ///< True when a camera position has been saved for this page.

public:
  /**
   * @brief  2D position of this node within the parent page's coordinate space.
   *
   * In List layout these coordinates describe the highlight bar position.
   * In Tile layout they describe the icon selection frame position.
   */
  typedef struct Position {
    float x, xTrg; ///< Current and target x coordinate.
    float y, yTrg; ///< Current and target y coordinate.
  } Position;

  Position position{}; ///< This node's animated position inside its parent menu.

  /**
   * @brief  Return the animated position of the child item at the given index.
   * @param  _index  Zero-based index of the child item.
   * @return Position struct for that child.
   */
  [[nodiscard]] Position getItemPosition(unsigned char _index) const;

  /**
   * @brief  Initialise all child item positions relative to the given camera state.
   * @param  _camera  Current camera state vector.
   */
  virtual void childPosInit(const std::vector<float> &_camera) {}

  /**
   * @brief  Initialise foreground element positions (e.g. scroll-bar, arrow).
   */
  virtual void forePosInit() {}

public:
  std::string title;             ///< Display title of this menu node.
  std::vector<unsigned char> pic; ///< Icon bitmap associated with this node.

protected:
  std::vector<unsigned char> picDefault = {}; // Fallback bitmap used when no icon is supplied.

  /**
   * @brief  Build and return the default placeholder bitmap.
   * @return Byte vector containing the default icon data.
   */
  [[nodiscard]] std::vector<unsigned char> generateDefaultPic();

public:
  Menu *parent{};                    ///< Pointer to the parent menu, or nullptr for the root.
  std::vector<Menu *> childMenu;     ///< Ordered list of child menu nodes (and widget-backed items).
  std::vector<Widget *> childWidget; ///< Widget associated with each child menu entry (1-to-1 by index).
  unsigned char selectIndex{};       ///< Index of the currently highlighted child item.

  /**
   * @brief  Return the total number of child items.
   * @return Child count.
   */
  [[nodiscard]] unsigned char getItemNum() const;

  /**
   * @brief  Return the child menu node that should be opened next (i.e. the selected one).
   * @return Pointer to the selected child Menu.
   */
  [[nodiscard]] Menu *getNextMenu() const;

  /**
   * @brief  Return the parent menu for back-navigation.
   * @return Pointer to the preview Menu.
   */
  [[nodiscard]] Menu *getPreview() const;

public:
  bool initFlag = false; ///< True after init() has been called; cleared by deInit().

public:
  /** @brief Default constructor. */
  Menu() = default;

  /** @brief Default destructor. */
  ~Menu() = default;

public:
  /**
   * @brief  Prepare this menu for display; must be called every time the menu is opened.
   * @param  _camera  Current camera state used to initialise child positions.
   */
  void init(const std::vector<float>& _camera);

  /**
   * @brief  Tear down this menu; must be called every time the menu is closed.
   */
  void deInit();

public:
  /**
   * @brief  Render all child items using the given camera offset.
   * @param  _camera  Current camera state vector.
   */
  virtual void render(const std::vector<float> &_camera) {}

public:
  /**
   * @brief  Append a child menu node with no associated widget.
   * @param  _page  Pointer to the child Menu to add.
   * @return True on success.
   */
  bool addItem(Menu *_page);

  /**
   * @brief  Append a child menu node paired with an interactive widget.
   * @param  _page       Pointer to the child Menu to add.
   * @param  _anyWidget  Widget to attach to this list entry.
   * @return True on success.
   */
  bool addItem(Menu *_page, Widget* _anyWidget);
};

/** @brief Vertical scrolling list layout with an optional sidebar progress bar. */
class List : public Menu {
public:
  /**
   * @brief  Return the layout type identifier.
   * @return "List".
   * @note   Overrides Menu::getType.
   */
  [[nodiscard]] std::string getType() const override { return "List"; }

public:
  // Animated positions for the foreground overlay elements (scroll bar).
  /** @brief Animated position of the foreground scroll-bar overlay. */
  typedef struct PositionForeground {
    float hBar, hBarTrg;  ///< Current and target height of the progress bar.
    float xBar, xBarTrg;  ///< Current and target x coordinate of the progress bar.
  } PositionForeground;

  PositionForeground positionForeground{}; ///< Foreground scroll-bar animation state.

public:
  /**
   * @brief  Initialise all child item positions for the list layout.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Menu::childPosInit.
   */
  void childPosInit(const std::vector<float> &_camera) override;

  /**
   * @brief  Initialise foreground scroll-bar position.
   * @note   Overrides Menu::forePosInit.
   */
  void forePosInit() override;

  /** @brief Default constructor. */
  List();

  /** @brief Constructs a List item with a text title.
   * @param  _title  Display title for this list entry.
   */
  explicit List(const std::string &_title);

  /** @brief Constructs a List item with an icon bitmap.
   * @param  _pic  Bitmap data for the icon.
   */
  explicit List(const std::vector<unsigned char>& _pic);

  /** @brief Constructs a List item with both a title and an icon.
   * @param  _title  Display title for this list entry.
   * @param  _pic    Bitmap data for the icon.
   */
  List(const std::string &_title, const std::vector<unsigned char>& _pic);

public:
  std::vector<unsigned char> boundary = {0, static_cast<unsigned char>(systemConfig.screenHeight / astraConfig.listLineHeight - 1)}; ///< First and last visible item indices (render window).

  /**
   * @brief  Return the current render-window boundary indices.
   * @return Two-element vector: {first visible index, last visible index}.
   */
  [[nodiscard]] std::vector<unsigned char> getBoundary() const { return boundary; }

  /**
   * @brief  Update the render-window boundary.
   * @param  _l  Index of the first visible item.
   * @param  _r  Index of the last visible item.
   */
  void refreshBoundary(unsigned char _l, unsigned char _r) { boundary = {_l, _r}; }

public:
  /**
   * @brief  Render all list entries and the scroll bar using the given camera offset.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Menu::render.
   */
  void render(const std::vector<float> &_camera) override;
};

/** @brief Horizontal icon-grid layout with animated arrow and dotted-line foreground. */
class Tile : public Menu {
public:
  /**
   * @brief  Return the layout type identifier.
   * @return "Tile".
   * @note   Overrides Menu::getType.
   */
  [[nodiscard]] std::string getType() const override { return "Tile"; }

public:
  // Animated positions for the foreground overlay elements (progress bar, arrow, dotted line).
  /** @brief Animated position of the foreground overlay elements for the tile grid. */
  typedef struct PositionForeground {
    float wBar, wBarTrg;                   ///< Current and target width of the progress bar.
    float yBar, yBarTrg;                   ///< Current and target y coordinate of the progress bar.
    float yArrow, yArrowTrg;               ///< Current and target y coordinate of the selection arrow.
    float yDottedLine, yDottedLineTrg;     ///< Current and target y coordinate of the dotted separator line.
  } PositionForeground;

  PositionForeground positionForeground{}; ///< Foreground overlay animation state.

public:
  /**
   * @brief  Initialise all child icon positions for the tile layout.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Menu::childPosInit.
   */
  void childPosInit(const std::vector<float> &_camera) override;

  /**
   * @brief  Initialise foreground arrow and dotted-line positions.
   * @note   Overrides Menu::forePosInit.
   */
  void forePosInit() override;

  /** @brief Default constructor. */
  Tile();

  /** @brief Constructs a Tile item with a text title.
   * @param  _title  Display title for this tile.
   */
  explicit Tile(const std::string &_title);

  /** @brief Constructs a Tile item with an icon bitmap.
   * @param  _pic  Bitmap data for the tile icon.
   */
  explicit Tile(const std::vector<unsigned char> &_pic);

  /** @brief Constructs a Tile item with both a title and an icon.
   * @param  _title  Display title for this tile.
   * @param  _pic    Bitmap data for the tile icon.
   */
  Tile(const std::string &_title, const std::vector<unsigned char> &_pic);

public:
  /**
   * @brief  Render all tile icons and the foreground overlay using the given camera offset.
   * @param  _camera  Current camera state vector.
   * @note   Overrides Menu::render.
   */
  void render(const std::vector<float> &_camera) override;
};

}

#endif //ASTRA_ASTRA__H
