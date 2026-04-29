//
// Created by Fir on 2024/2/2.
//

#include "launcher.h"

namespace astra {

void Launcher::renderFrame() {
  currentMenu->render(camera->getPosition());
  if (currentWidget != nullptr) currentWidget->render(camera->getPosition());
  selector->render(camera->getPosition());
  camera->update(currentMenu, selector);
}

bool Launcher::scanKeys(uint64_t nowMs) {
  if (nowMs - lastKeyScanMs < KEY_SCAN_INTERVAL_MS) return false;
  lastKeyScanMs = nowMs;
  HAL::keyScan();
  return true;
}

bool Launcher::openWidget(Menu* selectedMenu) {
  if (selectedMenu == nullptr || selectedMenu->childWidget.empty()) return false;

  currentWidget = selectedMenu->childWidget.front();
  if (currentWidget->getType() == "CheckBox") {
    static_cast<CheckBox *>(currentWidget)->toggle();
    currentWidget = nullptr;
  } else {
    currentWidget->init();
  }

  return true;
}

void Launcher::handleWidgetKey(unsigned char keyIndex, key::KEY_ACTION action) {
  if (currentWidget == nullptr) return;

  if (action == key::PRESS) {
    currentWidget = nullptr;
    return;
  }

  if (action != key::CLICK) return;

  if (currentWidget->getType() == "CheckBox") {
    static_cast<CheckBox *>(currentWidget)->toggle();
    currentWidget = nullptr;
  } else if (currentWidget->getType() == "PopUp") {
    if (keyIndex == key::KEY_0) static_cast<PopUp *>(currentWidget)->selectPreview();
    else if (keyIndex == key::KEY_1) static_cast<PopUp *>(currentWidget)->selectNext();
  } else if (currentWidget->getType() == "Slider") {
    if (keyIndex == key::KEY_0) static_cast<Slider *>(currentWidget)->sub();
    else if (keyIndex == key::KEY_1) static_cast<Slider *>(currentWidget)->add();
  }
}

void Launcher::handleMenuKey(unsigned char keyIndex, key::KEY_ACTION action) {
  if (action == key::CLICK) {
    if (keyIndex == key::KEY_0) selector->goPreview();
    else if (keyIndex == key::KEY_1) selector->goNext();
  } else if (action == key::PRESS) {
    if (keyIndex == key::KEY_0) close();
    else if (keyIndex == key::KEY_1) open();
  }
}

void Launcher::popInfo(std::string _info, uint16_t _time) {
  static bool init = false;
  static uint64_t beginTime = HAL::millis();
  static bool onRender = false;

  if (!init) {
    init = true;
    beginTime = HAL::millis();
    onRender = true;
  }

  float wPop = HAL::getFontWidth(_info) + 2 * getUIConfig().popMargin;
  float hPop = HAL::getFontHeight() + 2 * getUIConfig().popMargin;
  float yPop = 0 - hPop - 8;
  float yPopTrg = (HAL::getSystemConfig().screenHeight - hPop) / 3;
  float xPop = (HAL::getSystemConfig().screenWeight - wPop) / 2;

  while (onRender) {
    uint64_t nowMs = HAL::millis();

    HAL::canvasClear();
    renderFrame();

    HAL::setDrawType(0);
    HAL::drawRBox(xPop - 4, yPop - 4, wPop + 8, hPop + 8, getUIConfig().popRadius + 2);
    HAL::setDrawType(1);
    HAL::drawRFrame(xPop - 1, yPop - 1, wPop + 2, hPop + 2, getUIConfig().popRadius);
    HAL::drawEnglish(xPop + getUIConfig().popMargin,
                     yPop + getUIConfig().popMargin + HAL::getFontHeight(),
                     _info);

    HAL::canvasUpdate();

    Animation::move(&yPop, yPopTrg, getUIConfig().popSpeed);

    if (nowMs - beginTime >= _time) yPopTrg = 0 - hPop - 8;

    if (scanKeys(nowMs) && *HAL::getKeyFlag() == key::KEY_PRESSED) {
      for (unsigned char i = 0; i < key::KEY_NUM; i++) {
        if (HAL::getKeyMap()[i] == key::CLICK) yPopTrg = 0 - hPop - 8;
      }
      std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
      *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
    }

    if (yPop == 0 - hPop - 8) {
      onRender = false;
      init = false;
    }

    HAL::delay(1);
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

bool Launcher::open() {
  Menu* selectedMenu = currentMenu->getNextMenu();
  if (selectedMenu == nullptr) {
    popInfo("unreferenced page!", 600);
    return false;
  }

  if (openWidget(selectedMenu)) return true;

  if (selectedMenu->getItemNum() == 0) {
    popInfo("empty page!", 600);
    return false;
  }

  currentMenu->rememberCameraPos(camera->getPositionTrg());

  currentMenu->deInit();

  currentMenu = selectedMenu;
  currentMenu->forePosInit();
  currentMenu->childPosInit(camera->getPosition());

  selector->inject(currentMenu);

  return true;
}

bool Launcher::close() {
  if (currentWidget != nullptr) {
    currentWidget = nullptr;
    return true;
  }

  if (currentMenu->getPreview() == nullptr) {
    popInfo("unreferenced page!", 600);
    return false;
  }
  if (currentMenu->getPreview()->getItemNum() == 0) {
    popInfo("empty page!", 600);
    return false;
  }

  currentMenu->rememberCameraPos(camera->getPositionTrg());

  currentMenu->deInit();

  currentMenu = currentMenu->getPreview();
  currentMenu->forePosInit();
  currentMenu->childPosInit(camera->getPosition());

  selector->inject(currentMenu);

  return true;
}

void Launcher::update() {
  HAL::canvasClear();

  renderFrame();

  uint64_t nowMs = HAL::millis();
  scanKeys(nowMs);

  if (*HAL::getKeyFlag() == key::KEY_PRESSED) {
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
    for (unsigned char i = 0; i < key::KEY_NUM; i++) {
      key::KEY_ACTION action = HAL::getKeyMap()[i];
      if (action == key::INVALID) continue;

      if (currentWidget != nullptr) handleWidgetKey(i, action);
      else handleMenuKey(i, action);
    }
    std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
  }

  HAL::canvasUpdate();

  time++;
}
}
