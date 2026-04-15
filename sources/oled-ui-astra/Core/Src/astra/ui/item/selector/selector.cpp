/**
 * @file   selector.cpp
 * @brief  Implements the animated selection highlight box for List and Tile menus.
 * @author Fir
 * @date   2024-04-14
 */

#include "selector.h"

namespace astra {

void Selector::setPosition() {
  // Update target coordinates when the selection changes.
  if (menu->getType() == "Tile") {
    xTrg = menu->childMenu[menu->selectIndex]->position.xTrg - astraConfig.tileSelectBoxMargin;
    yTrg = menu->childMenu[menu->selectIndex]->position.yTrg - astraConfig.tileSelectBoxMargin;

    yText = systemConfig.screenHeight; // Reset tile title text: slide in from below the screen.
    yTextTrg = systemConfig.screenHeight - astraConfig.tileTextBottomMargin;

    wTrg = astraConfig.tileSelectBoxWidth;
    hTrg = astraConfig.tileSelectBoxHeight;
  } else if (menu->getType() == "List") {
    xTrg = menu->childMenu[menu->selectIndex]->position.xTrg - astraConfig.selectorMargin;
    yTrg = menu->childMenu[menu->selectIndex]->position.yTrg;

    wTrg = (float) HAL::getFontWidth(menu->childMenu[menu->selectIndex]->title) + astraConfig.listTextMargin * 2;
    hTrg = astraConfig.listLineHeight;
  }
}

// Moves the selector to item at _index; clamps to valid range.
void Selector::go(unsigned char _index) {
  Item::updateConfig();

  if (_index > menu->childMenu.size() - 1) return;
  if (_index < 0) return;
  menu->selectIndex = _index;

  setPosition();
}

void Selector::goNext() {
  if (this->menu->selectIndex == this->menu->childMenu.size() - 1) {
    if (astraConfig.menuLoop) go(0);
    else return;
  } else go(menu->selectIndex + 1);
  setPosition();
}

void Selector::goPreview() {
  if (this->menu->selectIndex == 0) {
    if (astraConfig.menuLoop) go(this->menu->childMenu.size() - 1);
    else return;
  } else go(menu->selectIndex - 1);
  setPosition();
}

bool Selector::inject(Menu *_menu) {
  if (_menu == nullptr) return false;

  this->menu = _menu;

  go(this->menu->selectIndex);  // Initialize selector position after injection.

  return true;
}

bool Selector::destroy() {
  if (this->menu == nullptr) return false;

  delete this->menu;
  this->menu = nullptr;
}

void Selector::render(std::vector<float> _camera) {
  Item::updateConfig();

  // Animate the selector box toward its target position and size.
  Animation::move(&x, xTrg, astraConfig.selectorXAnimationSpeed);
  Animation::move(&y, yTrg, astraConfig.selectorYAnimationSpeed);
  Animation::move(&h, hTrg, astraConfig.selectorHeightAnimationSpeed);
  Animation::move(&w, wTrg, astraConfig.selectorWidthAnimationSpeed);

  if (menu->getType() == "Tile") {
    Animation::move(&yText, yTextTrg, astraConfig.selectorYAnimationSpeed);

    // Draw the tile title text; text is not affected by camera offset.
    HAL::setDrawType(1);
    HAL::drawChinese((systemConfig.screenWeight -
      (float) HAL::getFontWidth(menu->childMenu[menu->selectIndex]->title)) / 2.0,
      yText + astraConfig.tileTitleHeight,
      menu->childMenu[menu->selectIndex]->title);

    // Draw the corner-bracket selection box; box is affected by camera offset.
    HAL::setDrawType(2);
    HAL::drawPixel(x + _camera[0], y + _camera[1]);
    // Top-left corner.
    HAL::drawHLine(x + _camera[0], y + _camera[1], astraConfig.tileSelectBoxLineLength + 1);
    HAL::drawVLine(x + _camera[0], y + _camera[1], astraConfig.tileSelectBoxLineLength + 1);
    // Bottom-left corner.
    HAL::drawHLine(x + _camera[0], y + _camera[1] + h - 1, astraConfig.tileSelectBoxLineLength + 1);
    HAL::drawVLine(x + _camera[0],
      y + _camera[1] + h - astraConfig.tileSelectBoxLineLength - 1,
      astraConfig.tileSelectBoxLineLength);
    // Top-right corner.
    HAL::drawHLine(x + _camera[0] + w - astraConfig.tileSelectBoxLineLength - 1,
      y + _camera[1],
      astraConfig.tileSelectBoxLineLength);
    HAL::drawVLine(x + _camera[0] + w - 1, y + _camera[1], astraConfig.tileSelectBoxLineLength + 1);
    // Bottom-right corner.
    HAL::drawHLine(x + _camera[0] + w - astraConfig.tileSelectBoxLineLength - 1,
      y + _camera[1] + h - 1,
      astraConfig.tileSelectBoxLineLength);
    HAL::drawVLine(x + _camera[0] + w - 1,
      y + _camera[1] + h - astraConfig.tileSelectBoxLineLength - 1,
      astraConfig.tileSelectBoxLineLength);

    HAL::drawPixel(x + _camera[0] + w - 1, y + _camera[1] + h - 1);
  } else if (menu->getType() == "List") {
    // Draw rounded selection box; affected by camera offset.
    HAL::setDrawType(2);
    HAL::drawRBox(x + _camera[0], y + _camera[1], w, h - 1, astraConfig.selectorRadius);
    HAL::setDrawType(1);
  }
}

std::vector<float> Selector::getPosition() {
  return {xTrg, yTrg};
}
}
