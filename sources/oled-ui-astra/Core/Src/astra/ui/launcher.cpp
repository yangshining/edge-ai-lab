/**
 * @file   launcher.cpp
 * @brief  Frame loop driver: renders the current menu, handles navigation input,
 *         and manages open/close transitions between pages.
 * @author Fir
 * @date   2024-02-02
 */

#include "launcher.h"

namespace astra {

void Launcher::popInfo(std::string _info, uint16_t _time) {
  static bool init = false;
  static unsigned long long int beginTime = this->time;
  static bool onRender = false;

  if (!init) {
    init = true;
    beginTime = this->time;
    onRender = true;
  }

  float wPop = HAL::getFontWidth(_info) + 2 * getUIConfig().popMargin;  // popup width
  float hPop = HAL::getFontHeight() + 2 * getUIConfig().popMargin;      // popup height
  float yPop = 0 - hPop - 8;  // -8 = off-screen above display (popup fully hidden)
  float yPopTrg = (HAL::getSystemConfig().screenHeight - hPop) / 3;     // target: upper-center
  float xPop = (HAL::getSystemConfig().screenWeight - wPop) / 2;        // horizontally centered

  while (onRender) {
    time++;

    HAL::canvasClear();
    // render one frame
    currentMenu->render(camera->getPosition());
    selector->render(camera->getPosition());
    camera->update(currentMenu, selector);
    // end frame

    HAL::setDrawType(0);
    HAL::drawRBox(xPop - 4, yPop - 4, wPop + 8, hPop + 8, getUIConfig().popRadius + 2);
    HAL::setDrawType(1);  // inverted color
    HAL::drawRFrame(xPop - 1, yPop - 1, wPop + 2, hPop + 2, getUIConfig().popRadius);  // rounded frame
    HAL::drawEnglish(xPop + getUIConfig().popMargin,
                     yPop + getUIConfig().popMargin + HAL::getFontHeight(),
                     _info);  // draw text

    HAL::canvasUpdate();

    Animation::move(&yPop, yPopTrg, getUIConfig().popSpeed);  // animate

    // slide out after timeout; a key press also dismisses the popup
    if (time - beginTime >= _time) yPopTrg = 0 - hPop - 8;  // slide out

    HAL::keyScan();
    if (HAL::getAnyKey()) {
      for (unsigned char i = 0; i < key::KEY_NUM; i++)
        if (HAL::getKeyMap()[i] == key::CLICK) yPopTrg = 0 - hPop - 8;  // slide out on key
      std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
    }

    if (yPop == 0 - hPop - 8) {
      onRender = false;  // exit condition: popup is fully off-screen
      init = false;
    }
  }
}

void Launcher::init(Menu *_rootPage) {
  currentMenu = _rootPage;

  camera = new Camera(0, 0);
  _rootPage->childPosInit(camera->getPosition());

  selector = new Selector();
  selector->inject(_rootPage);

  camera->init(_rootPage->getType());
}

// Navigates into the child menu of the current selection; shows an error popup and returns false if none exists.
bool Launcher::open() {

  // if the selected item has no child menu, bail out
  if (currentMenu->getNextMenu() == nullptr) {
    popInfo("unreferenced page!", 600);
    return false;
  }
  if (currentMenu->getNextMenu()->getItemNum() == 0) {
    popInfo("empty page!", 600);
    return false;
  }

  currentMenu->rememberCameraPos(camera->getPositionTrg());

  currentMenu->deInit();  // run exit animation before advancing the pointer

  currentMenu = currentMenu->getNextMenu();
  currentMenu->forePosInit();
  currentMenu->childPosInit(camera->getPosition());

  selector->inject(currentMenu);

  return true;
}

// Closes the current menu and returns to parent; returns false if no parent exists.
bool Launcher::close() {
  if (currentMenu->getPreview() == nullptr) {
    popInfo("unreferenced page!", 600);
    return false;
  }
  if (currentMenu->getPreview()->getItemNum() == 0) {
    popInfo("empty page!", 600);
    return false;
  }

  currentMenu->rememberCameraPos(camera->getPositionTrg());

  currentMenu->deInit();  // run exit animation before retreating the pointer

  currentMenu = currentMenu->getPreview();
  currentMenu->forePosInit();
  currentMenu->childPosInit(camera->getPosition());

  selector->inject(currentMenu);

  return true;
}

void Launcher::update() {
  HAL::canvasClear();

  currentMenu->render(camera->getPosition());
  if (currentWidget != nullptr) currentWidget->render(camera->getPosition());
  selector->render(camera->getPosition());
  camera->update(currentMenu, selector);

  if (time > 2) {  // skip first 2 frames to let animations settle
    HAL::keyScan();
    time = 0;
  }

  if (*HAL::getKeyFlag() == key::KEY_PRESSED) {
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
    // key 0 = Up/Back, key 1 = Down/Confirm
    for (unsigned char i = 0; i < key::KEY_NUM; i++) {
      if (HAL::getKeyMap()[i] == key::CLICK) {
        if (i == 0) { selector->goPreview(); }  // move selector to the previous item
        else if (i == 1) { selector->goNext(); }  // move selector to the next item
      } else if (HAL::getKeyMap()[i] == key::PRESS) {
        if (i == 0) { close(); }  // close (go back to parent)
        else if (i == 1) { open(); }  // open the selected item
      }
    }
    std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;  // defensive clear: ensure key state is clean on exit
  }

  HAL::canvasUpdate();

  time = HAL::millis() / 1000;
}
}
