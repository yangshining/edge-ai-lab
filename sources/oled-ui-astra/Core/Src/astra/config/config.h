/**
 * @file   config.h
 * @brief  Global UI animation speeds and layout constants for the Astra UI system.
 * @author Fir
 * @date   2024-01-25
 */

#pragma once
#ifndef ASTRA_CORE_SRC_SYSTEM_H_
#define ASTRA_CORE_SRC_SYSTEM_H_

#include "../../hal/hal_dreamCore/components/oled/graph_lib/u8g2/u8g2.h"

namespace astra {

/** @brief All configurable parameters for the Astra UI renderer and layout engine. */
struct config {
  // Animation speeds (0–100; higher = faster exponential decay).
  float tileAnimationSpeed         = 70;  ///< Tile page scroll animation speed.
  float listAnimationSpeed         = 70;  ///< List page scroll animation speed.
  float selectorYAnimationSpeed    = 60;  ///< Selector box vertical movement speed.
  float selectorXAnimationSpeed    = 70;  ///< Selector box horizontal movement speed.
  float selectorWidthAnimationSpeed  = 70; ///< Selector box width interpolation speed.
  float selectorHeightAnimationSpeed = 55; ///< Selector box height interpolation speed.
  float windowAnimationSpeed       = 25;  ///< Sub-menu open/close animation speed.
  float sideBarAnimationSpeed      = 15;  ///< Sidebar progress bar animation speed.
  float fadeAnimationSpeed         = 100; ///< Fade-in/out animation speed.
  float cameraAnimationSpeed       = 80;  ///< Virtual viewport pan animation speed.
  float logoAnimationSpeed         = 70;  ///< Splash-screen logo animation speed.

  // Behaviour flags.
  bool tileUnfold     = true; ///< Enable unfold animation for tile menus.
  bool listUnfold     = true; ///< Enable unfold animation for list menus.

  bool menuLoop       = true; ///< Wrap around when navigating past the first/last item.

  bool backgroundBlur = true;  ///< Draw blurred background behind overlays.
  bool lightMode      = false; ///< Use light (inverted) colour scheme.

  // List layout constants (pixels).
  float listBarWeight      = 5;    ///< Width of the sidebar progress bar.
  float listTextHeight     = 8;    ///< Height of a single text glyph row.
  float listTextMargin     = 4;    ///< Left/right margin around list item text.
  float listLineHeight     = 16;   ///< Total height of one list row (text + padding).
  float selectorRadius     = 0.5f; ///< Corner radius of the selector highlight box.
  float selectorMargin     = 4;    ///< Left margin between selector box and item text.
  float selectorTopMargin  = 2;    ///< Top margin between selector box and item text.

  // Tile layout constants (pixels).
  float tilePicWidth       = 30; ///< Icon width in the tile grid.
  float tilePicHeight      = 30; ///< Icon height in the tile grid.
  float tilePicMargin      = 8;  ///< Horizontal margin between tile icons.
  float tilePicTopMargin   = 8;  ///< Top margin above tile icons.
  float tileArrowWidth     = 6;  ///< Width of the navigation arrow glyph.
  float tileArrowMargin    = 4;  ///< Margin around the navigation arrow.

  float tileDottedLineBottomMargin = 18; ///< Bottom margin of the dotted separator line (top: 46 px).
  float tileArrowBottomMargin      = 8;  ///< Bottom margin of the navigation arrow (top: 56 px).
  float tileTextBottomMargin       = 12; ///< Bottom margin of the tile title label (top: 52 px).

  float tileBarHeight = 2; ///< Height of the tile-view progress bar.

  float tileSelectBoxLineLength = 5;  ///< Corner tick length of the tile selection box.
  float tileSelectBoxMargin     = 3;  ///< Margin between the tile icon and its selection box.
  float tileSelectBoxWidth      = tileSelectBoxMargin * 2 + tilePicWidth;  ///< Computed selection box width.
  float tileSelectBoxHeight     = tileSelectBoxMargin * 2 + tilePicHeight; ///< Computed selection box height.
  float tileTitleHeight         = 8;  ///< Height of the tile title text glyph row.

  float tileBtnMargin = 16; ///< Margin around tile-view navigation buttons.

  // Pop-up overlay layout constants (pixels).
  float popMargin = 4;  ///< Inner margin inside a pop-up dialog.
  float popRadius = 2;  ///< Corner radius of a pop-up dialog.
  float popSpeed  = 90; ///< Pop-up open/close animation speed.

  // Logo / splash-screen constants.
  float logoStarLength      = 2;  ///< Half-length of each decorative star ray.
  float logoTextHeight      = 14; ///< Height of the large logo title text.
  float logoCopyRightHeight = 8;  ///< Height of the copyright line text.
  unsigned char logoStarNum = 16; ///< Number of decorative stars on the splash screen.

  // Fonts used by the UI. Change these if not using the u8g2 library.
  const unsigned char *logoTitleFont    = u8g2_font_Cascadia; ///< Font for the splash-screen title.
  const unsigned char *logoCopyRightFont = u8g2_font_myfont;  ///< Font for the copyright line.
  const unsigned char *mainFont          = u8g2_font_myfont;  ///< Font for all other UI text.

  // CheckBox widget layout constants (pixels).
  float checkBoxWidth       = 8;  ///< CheckBox widget width.
  float checkBoxHeight      = 8;  ///< CheckBox widget height.
  float checkBoxTopMargin   = 4;  ///< Distance from the top edge of the list row to the check box.
  float checkBoxRightMargin = 10; ///< Distance from the right edge of the screen to the check box.
  float checkBoxRadius      = 1;  ///< Corner radius of the check box.
};

/**
 * @brief  Return a reference to the global UI configuration singleton.
 * @return Reference to the single shared @ref config instance.
 */
static config &getUIConfig() {
  static config astraConfig;
  return astraConfig;
}

} // namespace astra

#endif //ASTRA_CORE_SRC_SYSTEM_H_
