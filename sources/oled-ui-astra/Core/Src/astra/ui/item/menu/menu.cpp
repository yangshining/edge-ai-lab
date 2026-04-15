/**
 * @file   menu.cpp
 * @brief  Implements the Menu base class and the List and Tile concrete layout types,
 *         including position initialisation, child management, and per-frame rendering.
 * @author Fir
 * @date   2024-01-21
 */

#include "menu.h"

namespace astra {
Menu::Position Menu::getItemPosition(unsigned char _index) const { return childMenu[_index]->position; }

std::vector<unsigned char> Menu::generateDefaultPic() {
  this->picDefault.resize(120, 0xFF);
  return this->picDefault;
}

unsigned char Menu::getItemNum() const { return childMenu.size(); }

Menu *Menu::getNextMenu() const { return childMenu[selectIndex]; }

Menu *Menu::getPreview() const { return parent; }

void Menu::init(const std::vector<float>& _camera) { }

void Menu::deInit() {
  // TODO: deInit is not fully implemented; only Animation::exit() is called.
  Animation::exit();
}

bool Menu::addItem(Menu *_page) {
  if (_page == nullptr) return false;
  if (!_page->childWidget.empty()) return false;

  _page->parent = this;
  this->childMenu.push_back(_page);
  this->forePosInit();
  return true;
}

bool Menu::addItem(Menu *_page, Widget *_anyWidget) {
  if (_anyWidget == nullptr) return false;
  if (this->addItem(_page)) {
    _page->childWidget.push_back(_anyWidget);
    _anyWidget->parent = _page;
    _anyWidget->init();
    return true;
  } else return false;
}

void List::childPosInit(const std::vector<float> &_camera) {
  unsigned char _index = 0;

  for (auto _iter : childMenu) {
    _iter->position.x = astraConfig.listTextMargin;
    _iter->position.xTrg = astraConfig.listTextMargin;
    _iter->position.yTrg = _index * astraConfig.listLineHeight;

    _index++;

    // position init that is affected by the unfold toggle
    // the root page has an opening animation so it does not need to unfold from the top
    if (_iter->parent->parent == nullptr) { _iter->position.y = _iter->position.yTrg; continue; }
    if (astraConfig.listUnfold) { _iter->position.y = _camera[1] - astraConfig.listLineHeight;
      continue; }  // text unfold from top.
  }
}

void List::forePosInit() {
  positionForeground.xBarTrg = systemConfig.screenWeight - astraConfig.listBarWeight;

  // position init that is affected by the unfold toggle
  if (astraConfig.listUnfold) positionForeground.hBar = 0;  // bar unfold from top.
  else positionForeground.hBar = positionForeground.hBarTrg;

  // position init that always runs
  positionForeground.xBar = systemConfig.screenWeight;
}

List::List() {
  this->title = "-unknown";
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

List::List(const std::string &_title) {
  this->title = _title;
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

List::List(const std::vector<unsigned char> &_pic) {
  this->title = "-unknown";
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

List::List(const std::string &_title, const std::vector<unsigned char> &_pic) {
  this->title = _title;
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

void List::render(const std::vector<float> &_camera) {
  Item::updateConfig();

  HAL::setDrawType(1);
  // Items may lie outside screen bounds (positive or negative) during animation — render them anyway so
  // Animation::move() continues advancing their position toward the target each frame.
  // scan all children, draw text and widget on the list.
  for (auto _iter : childMenu) {
    // draw the widget indicator inside the list row
    if (!_iter->childWidget.empty()) {
      for (auto _widget : _iter->childWidget) {
        _widget->renderIndicator(
            systemConfig.screenWeight - astraConfig.checkBoxRightMargin - astraConfig.checkBoxWidth,
            _iter->position.y + astraConfig.checkBoxTopMargin,
            _camera);
      }
    }
    // draw the item label
    HAL::drawChinese(_iter->position.x + _camera[0],
                     _iter->position.y + astraConfig.listTextHeight +
                     astraConfig.listTextMargin + _camera[1],
                     _iter->title);
    // yTrg is already determined when addItem() is called
    Animation::move(&_iter->position.y, _iter->position.yTrg, astraConfig.listAnimationSpeed);
  }

  // Update bar target height.
  positionForeground.hBarTrg = (selectIndex + 1) * ((float) systemConfig.screenHeight / getItemNum());
  // draw the indicator lines
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.listBarWeight, 0, astraConfig.listBarWeight);
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.listBarWeight,
                 systemConfig.screenHeight - 1,
                 astraConfig.listBarWeight);
  HAL::drawVLine(systemConfig.screenWeight - ceil((float) astraConfig.listBarWeight / 2.0f),
                 0,
                 systemConfig.screenHeight);
  // draw bar.
  HAL::drawBox(positionForeground.xBar, 0, astraConfig.listBarWeight, positionForeground.hBar);

  // light mode.
  if (astraConfig.lightMode) {
    HAL::setDrawType(2);
    HAL::drawBox(0, 0, systemConfig.screenWeight, systemConfig.screenHeight);
    HAL::setDrawType(1);
  }

  Animation::move(&positionForeground.hBar, positionForeground.hBarTrg, astraConfig.listAnimationSpeed);
  Animation::move(&positionForeground.xBar, positionForeground.xBarTrg, astraConfig.listAnimationSpeed);
}

void Tile::childPosInit(const std::vector<float> &_camera) {
  unsigned char _index = 0;

  for (auto _iter : childMenu) {
    _iter->position.y = 0;
    _iter->position.xTrg = systemConfig.screenWeight / 2 - astraConfig.tilePicWidth / 2 +
                           (_index) * (astraConfig.tilePicMargin + astraConfig.tilePicWidth);
    _iter->position.yTrg = astraConfig.tilePicTopMargin;

    _index++;

    if (_iter->parent->parent == nullptr) { _iter->position.x = _iter->position.xTrg; continue; }
    if (astraConfig.tileUnfold) { _iter->position.x = _camera[0] - astraConfig.tilePicWidth; continue; }  // unfold from left.
  }
}

void Tile::forePosInit() {
  positionForeground.yBarTrg = 0;
  positionForeground.yArrowTrg = systemConfig.screenHeight - astraConfig.tileArrowBottomMargin;
  positionForeground.yDottedLineTrg = systemConfig.screenHeight - astraConfig.tileDottedLineBottomMargin;

  if (astraConfig.tileUnfold) positionForeground.wBar = 0;  // bar unfold from left.
  else positionForeground.wBar = positionForeground.wBarTrg;

  // always-run position init
  // bottom arrow and dotted-line initial positions (off the bottom of the screen)
  positionForeground.yArrow = systemConfig.screenHeight;
  positionForeground.yDottedLine = systemConfig.screenHeight;

  // top progress bar slides in from above (coordinate, not height, animates from off-screen)
  positionForeground.yBar = 0 - astraConfig.tileBarHeight;
}

Tile::Tile() {
  this->title = "-unknown";
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

Tile::Tile(const std::string &_title) {
  this->title = _title;
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

Tile::Tile(const std::vector<unsigned char> &_pic) {
  this->title = "-unknown";
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

Tile::Tile(const std::string &_title, const std::vector<unsigned char> &_pic) {
  this->title = _title;
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

void Tile::render(const std::vector<float> &_camera) {
  Item::updateConfig();

  HAL::setDrawType(1);
  // draw pic.
  for (auto _iter : childMenu) {
    HAL::drawBMP(_iter->position.x + _camera[0],
                 astraConfig.tilePicTopMargin + _camera[1],
                 astraConfig.tilePicWidth,
                 astraConfig.tilePicHeight,
                 _iter->pic.data());
    // xTrg is already determined when addItem() is called
    Animation::move(&_iter->position.x,
                    _iter->position.xTrg,
                    astraConfig.tileAnimationSpeed);
  }

  // Update bar target width.
  positionForeground.wBarTrg = (selectIndex + 1) * ((float) systemConfig.screenWeight / getItemNum());
  HAL::drawBox(0, positionForeground.yBar, positionForeground.wBar, astraConfig.tileBarHeight);

  // draw left arrow.
  HAL::drawHLine(astraConfig.tileArrowMargin, positionForeground.yArrow, astraConfig.tileArrowWidth);
  HAL::drawPixel(astraConfig.tileArrowMargin + 1, positionForeground.yArrow + 1);
  HAL::drawPixel(astraConfig.tileArrowMargin + 2, positionForeground.yArrow + 2);
  HAL::drawPixel(astraConfig.tileArrowMargin + 1, positionForeground.yArrow - 1);
  HAL::drawPixel(astraConfig.tileArrowMargin + 2, positionForeground.yArrow - 2);

  // draw right arrow.
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.tileArrowWidth - astraConfig.tileArrowMargin,
                 positionForeground.yArrow,
                 astraConfig.tileArrowWidth);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth, positionForeground.yArrow + 1);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth - 1, positionForeground.yArrow + 2);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth, positionForeground.yArrow - 1);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth - 1, positionForeground.yArrow - 2);

  // draw left button.
  HAL::drawHLine(astraConfig.tileBtnMargin, positionForeground.yArrow + 2, 9);
  HAL::drawBox(astraConfig.tileBtnMargin + 2, positionForeground.yArrow + 2 - 4, 5, 4);

  // draw the right button.
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.tileBtnMargin - 9, positionForeground.yArrow + 2, 9);
  HAL::drawBox(systemConfig.screenWeight - astraConfig.tileBtnMargin - 9 + 2,
               positionForeground.yArrow + 2 - 4,
               5,
               4);

  // draw dotted line.
  HAL::drawHDottedLine(0, positionForeground.yDottedLine, systemConfig.screenWeight);

  Animation::move(&positionForeground.yDottedLine, positionForeground.yDottedLineTrg, astraConfig.tileAnimationSpeed);
  Animation::move(&positionForeground.yArrow, positionForeground.yArrowTrg, astraConfig.tileAnimationSpeed);
  Animation::move(&positionForeground.wBar, positionForeground.wBarTrg, astraConfig.tileAnimationSpeed);
  Animation::move(&positionForeground.yBar, positionForeground.yBarTrg, astraConfig.tileAnimationSpeed);
}
}
