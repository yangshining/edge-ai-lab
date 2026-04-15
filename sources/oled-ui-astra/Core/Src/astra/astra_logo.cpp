/**
 * @file   astra_logo.cpp
 * @brief  Renders the animated splash screen with stars, title, and copyright text.
 * @author Fir
 * @date   2024-03-21
 */

#include <cmath>
#include "astra_logo.h"

namespace astra {

void drawLogo(uint16_t _time) {
  auto animation = [] (float &_pos, float _posTrg, float _speed) -> void {
    if (_pos != _posTrg) {
      if (std::fabs(_pos - _posTrg) < 0.15f) _pos = _posTrg;
      else _pos += (_posTrg - _pos) / ((100 - _speed) / 1.0f);
    }
  };

  static bool onRender = true;
  static bool isInit = false;

  static uint16_t time = 0;

  while(onRender) {
    time++;

    static std::vector<float> yStars;
    static std::vector<float> yStarsTrg;
    static std::vector<float> xStars;

    static std::string text = "astra UI";
    static std::string copyRight = "powered by";

    HAL::setFont(getUIConfig().logoTitleFont);
    static float xTitle = (HAL::getSystemConfig().screenWeight - HAL::getFontWidth(text)) / 2;
    HAL::setFont(getUIConfig().logoCopyRightFont);
    static float xCopyRight = (HAL::getSystemConfig().screenWeight - HAL::getFontWidth(copyRight)) / 2;
    HAL::setFont(getUIConfig().mainFont);

    // All positions are top-left corner coordinates.
    static float yTitle = 0 - getUIConfig().logoTextHeight - 1;
    static float yCopyRight = 0 - getUIConfig().logoCopyRightHeight - 1;
    static float yTitleTrg = 0;
    static float yCopyRightTrg = 0;

    static float xBackGround = 0;
    static float yBackGround = 0 - HAL::getSystemConfig().screenHeight - 1;

    static float yBackGroundTrg = 0;

    if (time < _time) {
      yBackGroundTrg = 0;
      // Initialize star positions once; star coordinates represent centre points.
      if (!isInit) {
        yStars.clear();
        yStarsTrg.clear();
        xStars.clear();

        for (unsigned char i = 0; i < getUIConfig().logoStarNum; i++) {
          // Seed the random number generator.
          srand(HAL::getRandomSeed() * 7);

          yStars.push_back(0 - getUIConfig().logoStarLength - 1);

          // Random Y in [1, screenHeight - 2*starLength - 2].
          yStarsTrg.push_back(1 + rand() % (uint16_t)(HAL::getSystemConfig().screenHeight - 2 * getUIConfig().logoStarLength - 2 + 1));
          // Random X in [1, screenWeight - 2*starLength - 2].
          xStars.push_back(1 + rand() % (uint16_t)(HAL::getSystemConfig().screenWeight - 2 * getUIConfig().logoStarLength - 2 + 1));
        }
        isInit = true;
      }
      yTitleTrg = HAL::getSystemConfig().screenHeight / 2 - getUIConfig().logoTextHeight / 2;  // Center vertically.
      yCopyRightTrg = yTitleTrg - getUIConfig().logoCopyRightHeight - 4;
    } else {
      // Exit: background, stars, and text all animate out together.
      yBackGroundTrg = 0 - HAL::getSystemConfig().screenHeight - 1;
      yStarsTrg.assign(getUIConfig().logoStarNum, 0 - getUIConfig().logoStarLength - 1);
      yTitleTrg = 0 - getUIConfig().logoTextHeight - 1;
      yCopyRightTrg = 0 - getUIConfig().logoCopyRightHeight - 1;
    }

    // Render order: background mask enters first, then stars, then text.
    // All elements exit together.
    HAL::canvasClear();

    HAL::setDrawType(0);
    // Background mask.
    HAL::drawBox(xBackGround, yBackGround, HAL::getSystemConfig().screenWeight, HAL::getSystemConfig().screenHeight);
    animation(yBackGround, yBackGroundTrg, getUIConfig().logoAnimationSpeed);
    HAL::setDrawType(1);
    HAL::drawHLine(0, yBackGround + HAL::getSystemConfig().screenHeight, HAL::getSystemConfig().screenWeight);

    // Draw stars (cross shape: two horizontal lines and two vertical lines).
    for (unsigned char i = 0; i < getUIConfig().logoStarNum; i++) {
      HAL::drawHLine(xStars[i] - getUIConfig().logoStarLength - 1, yStars[i], getUIConfig().logoStarLength);
      HAL::drawHLine(xStars[i] + 2, yStars[i], getUIConfig().logoStarLength);
      HAL::drawVLine(xStars[i], yStars[i] - getUIConfig().logoStarLength - 1, getUIConfig().logoStarLength);
      HAL::drawVLine(xStars[i], yStars[i] + 2, getUIConfig().logoStarLength);

      animation(yStars[i], yStarsTrg[i], getUIConfig().logoAnimationSpeed);
    }

    HAL::setFont(getUIConfig().logoTitleFont);
    HAL::drawEnglish(xTitle, yTitle + getUIConfig().logoTextHeight, text);
    HAL::setFont(getUIConfig().logoCopyRightFont);
    HAL::drawEnglish(xCopyRight, yCopyRight + getUIConfig().logoCopyRightHeight, copyRight);
    animation(yTitle, yTitleTrg, getUIConfig().logoAnimationSpeed);
    animation(yCopyRight, yCopyRightTrg, getUIConfig().logoAnimationSpeed);

    HAL::canvasUpdate();

    if (time >= _time && yBackGround == 0 - HAL::getSystemConfig().screenHeight - 1) onRender = false;
  }
}
}
