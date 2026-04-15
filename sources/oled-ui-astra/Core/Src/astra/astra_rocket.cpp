/**
 * @file   astra_rocket.cpp
 * @brief  Entry point: builds the demo menu tree, injects the HAL, and drives the main loop.
 * @author Fir
 * @date   2024-03-07
 */

// TODO: flash usage is at ~90.93%, approaching the 128 KB limit.

#include <vector>
#include "astra_rocket.h"
#include "astra_logo.h"

auto *astraLauncher = new astra::Launcher();
auto *rootPage = new astra::Tile("root");

// Demo/test widget state — replace with real application state.
bool test = false;
unsigned char testIndex = 0;
unsigned char testSlider = 60;

auto *secondPage = new astra::List("secondPage");

void astraCoreInit(void) {
  HAL::inject(new HALDreamCore);

  HAL::delay(350);
  astra::drawLogo(1000);

  rootPage->addItem(new astra::List("test1"));
  rootPage->addItem(new astra::List("测试2"));
  rootPage->addItem(new astra::List("测试测试3"));
  rootPage->addItem(new astra::List("测试测试3"));
  rootPage->addItem(secondPage);

  secondPage->addItem(new astra::List());
  secondPage->addItem(new astra::List("-测试2"), new astra::CheckBox(test));
  secondPage->addItem(new astra::Tile("-测试测试3"), new astra::PopUp(1, "测试", {"测试"}, testIndex));
  secondPage->addItem(new astra::Tile("-测试测试测试4"), new astra::Slider("测试", 0, 100, 50, testSlider));
  secondPage->addItem(new astra::List("-测试测试测试5"));
  // Placeholder menu items — replace with real pages.
  secondPage->addItem(new astra::List("-测试测试测试6"));
  secondPage->addItem(new astra::List("-测试测试测试6"));
  secondPage->addItem(new astra::List("-测试测试测试6"));
  secondPage->addItem(new astra::List("-测试测试测试6"));

  astraLauncher->init(rootPage);
}

void astraCoreStart(void) {
  for (;;) {  //NOLINT
    astraLauncher->update();
  }
}

void astraCoreDestroy(void) {
  HAL::destroy();
  delete astraLauncher;
}
