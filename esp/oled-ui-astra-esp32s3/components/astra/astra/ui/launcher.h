//
// Created by Fir on 2024/2/2.
//
#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_
#define ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_

#include "item/menu/menu.h"
#include "item/selector/selector.h"
#include "item/camera/camera.h"

namespace astra {

class Launcher {
private:
  static constexpr uint64_t KEY_SCAN_INTERVAL_MS = 10;

  Menu* currentMenu;
  Widget* currentWidget = nullptr;
  Selector* selector;
  Camera* camera;

  uint64_t time = 0;
  uint64_t lastKeyScanMs = 0;

  void renderFrame();
  bool scanKeys(uint64_t nowMs);
  bool openWidget(Menu* selectedMenu);
  void handleWidgetKey(unsigned char keyIndex, key::KEY_ACTION action);
  void handleMenuKey(unsigned char keyIndex, key::KEY_ACTION action);

public:
  void popInfo(std::string _info, uint16_t _time);

  void init(Menu* _rootPage);

  bool open();
  bool close();

  void update();

  Camera* getCamera() { return camera; }
  Selector* getSelector() { return selector; }
};
}

#endif //ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_
