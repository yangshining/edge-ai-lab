//
// Created by Fir on 2024/3/7 007.
//

//todo warning flash used age >= 90.93%, will be damaged soon

#include <vector>
#include "astra_rocket.h"
#include "astra_logo.h"
#include "hal_esp32s3.h"  // concrete HAL — only this translation unit needs it

auto *astraLauncher = new astra::Launcher();
auto *rootPage = new astra::Tile("root");

bool test = false;
unsigned char testIndex = 0;
unsigned char testSlider = 60;

auto *secondPage = new astra::List("secondPage");

void astraCoreInit(void) {
  HAL::inject(new HALEspCore);

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
  secondPage->addItem(new astra::List("-测试测试测试6"));
  secondPage->addItem(new astra::List("-测试测试测试6"));
  secondPage->addItem(new astra::List("-测试测试测试6"));
  secondPage->addItem(new astra::List("-测试测试测试6"));

  astraLauncher->init(rootPage);
}

void astraCoreStart(void) {
  for (;;) {  //NOLINT
    astraLauncher->update();
    HAL::delay(1);
//    astraCoreTest();
  }
}

void astraCoreDestroy(void) {
  HAL::destroy();
  delete astraLauncher;
}
