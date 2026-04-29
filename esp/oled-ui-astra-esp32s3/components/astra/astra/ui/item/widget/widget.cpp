#include "widget.h"

namespace astra {

CheckBox::CheckBox(bool &_value) {
  value = _value;
  if (value) isCheck = true;
  else isCheck = false;
  this->parent = nullptr;
}

bool CheckBox::check() {
  value = true;
  isCheck = true;
  return isCheck;
}

bool CheckBox::uncheck() {
  value = false;
  isCheck = false;
  return isCheck;
}

bool CheckBox::toggle() {
  value = !value;
  isCheck = !isCheck;
  return isCheck;
}

void CheckBox::init() {
  if (value) isCheck = true;
  else isCheck = false;
}

void CheckBox::deInit() {
  delete this;
}

void CheckBox::renderIndicator(float _x, float _y, const std::vector<float> &_camera) {
  Item::updateConfig();
  HAL::setDrawType(1);
  HAL::drawRFrame(_x + _camera[0],
                  _y + _camera[1],
                  astraConfig.checkBoxWidth,
                  astraConfig.checkBoxHeight,
                  astraConfig.checkBoxRadius);
  if (isCheck) {
    HAL::drawBox(_x + _camera[0] + astraConfig.checkBoxWidth / 4,
                 _y + _camera[1] + astraConfig.checkBoxHeight / 4,
                 astraConfig.checkBoxWidth / 2,
                 astraConfig.checkBoxHeight / 2);
  }
}

void CheckBox::render(const std::vector<float> &_camera) {
}

PopUp::PopUp(unsigned char _direction,
             const std::string &_title,
             const std::vector<std::string> &_options,
             unsigned char &_value) {
  direction = _direction;
  title = _title;
  options = _options;
  boundary = options.size();
  value = _value;
  this->parent = nullptr;
}

void PopUp::selectNext() {
  if (boundary == 0) return;
  if (value == boundary - 1) value = 0;
  else value++;
}

void PopUp::selectPreview() {
  if (boundary == 0) return;
  if (value == 0) value = boundary - 1;
  else value--;
}

bool PopUp::select(unsigned char _index) {
  if (boundary == 0 || _index > boundary - 1) return false;
  value = _index;
  return true;
}

void PopUp::init() { }

void PopUp::deInit() {
  delete this;
}

void PopUp::renderIndicator(float _x, float _y, const std::vector<float> &_camera) {
  Item::updateConfig();
  HAL::setDrawType(1);
  HAL::drawEnglish(_x + _camera[0] + 1, _y + _camera[1] + astraConfig.listTextHeight, std::to_string(value));
}

void PopUp::render(const std::vector<float> &_camera) {
  Item::updateConfig();

  const float width = systemConfig.screenWeight * 0.75f;
  const float height = HAL::getFontHeight() + 2 * astraConfig.popMargin;
  const float x = (systemConfig.screenWeight - width) / 2.0f;
  const float y = (systemConfig.screenHeight - height) / 2.0f;

  HAL::setDrawType(0);
  HAL::drawRBox(x - 2, y - 2, width + 4, height + 4, astraConfig.popRadius + 1);
  HAL::setDrawType(1);
  HAL::drawRFrame(x, y, width, height, astraConfig.popRadius);

  std::string text = title;
  if (!options.empty() && value < options.size()) text += ": " + options[value];
  HAL::drawChinese(x + astraConfig.popMargin,
                   y + astraConfig.popMargin + HAL::getFontHeight(),
                   text);
}

Slider::Slider(const std::string &_title,
               unsigned char _min,
               unsigned char _max,
               unsigned char _step,
               unsigned char &_value) {
  title = _title;
  maxLength = 0;
  min = _min;
  max = _max;
  step = _step;
  value = _value;
  lengthIndicator = 0;
  this->parent = nullptr;

  if (value > max) valueOverflow = true;
  else valueOverflow = false;
}

unsigned char Slider::add() {
  if (value > max - step) value = max;
  else value += step;
  refreshLayout();
  return this->value;
}

unsigned char Slider::sub() {
  if (value < min + step) value = min;
  else value -= step;
  refreshLayout();
  return this->value;
}

void Slider::refreshLayout() {
  maxLength = std::floor(HAL::getSystemConfig().screenWeight * 0.6);
  if (max <= min) {
    position.lTrg = 0;
    lengthIndicator = 0;
    return;
  }

  if (value > max) value = max;
  if (value < min) value = min;

  const float ratio = (float)(value - min) / (max - min);
  position.lTrg = std::floor(ratio * maxLength);
  lengthIndicator = std::round(ratio * 6);
}

void Slider::init() {
  refreshLayout();
}

void Slider::deInit() {
  delete this;
}

void Slider::renderIndicator(float _x, float _y, const std::vector<float> &_camera) {
  Item::updateConfig();
  HAL::setDrawType(1);
  HAL::drawRFrame(_x + _camera[0] - 1, _y + _camera[1] - 1, 10, 8, 1);
  HAL::drawBox(_x + _camera[0] + 1, _y + _camera[1] + 1, lengthIndicator, 4);
}

void Slider::render(const std::vector<float> &_camera) {
  Item::updateConfig();
  Animation::move(&position.l, position.lTrg, astraConfig.windowAnimationSpeed);

  const float x = (systemConfig.screenWeight - maxLength) / 2.0f;
  const float y = systemConfig.screenHeight / 2.0f;

  HAL::setDrawType(0);
  HAL::drawRBox(x - 4, y - HAL::getFontHeight() - 8, maxLength + 8, HAL::getFontHeight() + 18, 2);
  HAL::setDrawType(1);
  HAL::drawChinese(x, y - 6, title + ": " + std::to_string(value));
  HAL::drawFrame(x, y, maxLength, 8);
  HAL::drawBox(x + 1, y + 1, position.l, 6);
}
}
