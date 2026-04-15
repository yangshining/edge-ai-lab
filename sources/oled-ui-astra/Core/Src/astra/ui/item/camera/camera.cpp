/**
 * @file   camera.cpp
 * @brief  Implements the Camera viewport that offsets all rendered coordinates.
 * @author Fir
 * @date   2024-04-14
 */

#include "camera.h"

#include <cmath>

namespace astra {
Camera::Camera() {
  this->xInit = 0;
  this->yInit = 0;

  this->x = 0;
  this->y = 0;

  this->xTrg = 0;
  this->yTrg = 0;
}

// Camera coordinates are stored as negatives of the world position.
// At render time each item's position is added to the camera offset,
// so moving the camera "down" shifts items visually upward.
// Callers pass positive world coordinates; the constructor negates them.

Camera::Camera(float _x, float _y) {
  this->xInit = 0 - _x;
  this->yInit = 0 - _y;

  this->x = 0 - _x;
  this->y = 0 - _y;

  this->xTrg = 0 - _x;
  this->yTrg = 0 - _y;
}

// Returns true if the point (_x, _y) is outside the current viewport bounds.
unsigned char Camera::outOfView(float _x, float _y) {
  if (_x < 0 - this->x || _y < 0 - this->y) return 1;
  if (_x > (0 - this->x) + systemConfig.screenWeight - 1 || _y > (0 - this->y) + systemConfig.screenHeight - 1) return 2;
  return 0;
}

std::vector<float> Camera::getPosition() {
  return {x, y};
}

std::vector<float> Camera::getPositionTrg() {
  return {xTrg, yTrg};
}

void Camera::init(const std::string &_type) {
  if (_type == "List") {
    this->goDirect(0, static_cast<float>((0 - sys::getSystemConfig().screenHeight) * 10));
  }
  else if (_type == "Tile") {
    this->goDirect(static_cast<float>((0 - sys::getSystemConfig().screenWeight) * 10), 0);
  }
}

// Moves the camera toward (_x, _y) using exponential-decay animation.
void Camera::go(float _x, float _y) {
  this->xTrg = 0 - _x;
  this->yTrg = 0 - _y;
}

void Camera::go(const std::vector<float> &_pos) {
  this->xTrg = 0 - _pos[0];
  this->yTrg = 0 - _pos[1];
}

void Camera::goDirect(float _x, float _y) {
  this->x = 0 - _x;
  this->y = 0 - _y;
  this->xTrg = 0 - _x;
  this->yTrg = 0 - _y;
}

void Camera::move(float _x, float _y) {
  this->xTrg -= _x;
  this->yTrg -= _y;
}

void Camera::moveDirect(float _x, float _y) {
  this->x -= _x;
  this->y -= _y;
  this->xTrg -= _x;
  this->yTrg -= _y;
}

void Camera::goToListItemRolling(List *_menu) {
  static const unsigned char maxItemPerPage = systemConfig.screenHeight / astraConfig.listLineHeight;

  // Initialise camera position on first entry only; subsequent entries skip re-centring.
  if (!_menu->initFlag) {
    go(0,0);
    _menu->initFlag = true;
  }

  if (_menu->selectIndex < _menu->getBoundary()[0]) {
    // Use move (relative) rather than go (absolute) here.
    move(0, (_menu->selectIndex - _menu->getBoundary()[0]) * astraConfig.listLineHeight);
    _menu->refreshBoundary(_menu->selectIndex, _menu->selectIndex + maxItemPerPage - 1);
    return;
  }
  else if (_menu->selectIndex > _menu->getBoundary()[1]) {
    move(0, (_menu->selectIndex - _menu->getBoundary()[1]) * astraConfig.listLineHeight);
    _menu->refreshBoundary(_menu->selectIndex - maxItemPerPage + 1, _menu->selectIndex);
    return;
  }
  else return;
}

void Camera::goToTileItem(unsigned char _index) {
  go(_index * (astraConfig.tilePicWidth + astraConfig.tilePicMargin), 0);
}

void Camera::reset() {
  go(this->xInit, this->yInit);
}

void Camera::resetDirect() {
  goDirect(this->xInit, this->yInit);
}

void Camera::render() {
  Animation::move(&this->x, this->xTrg, astraConfig.cameraAnimationSpeed);
  Animation::move(&this->y, this->yTrg, astraConfig.cameraAnimationSpeed);
}

void Camera::update(Menu *_menu, Selector *_selector) {

  if (_menu->cameraPosMemoryFlag) {
    go(0 - _menu->getCameraMemoryPos()[0], 0 - _menu->getCameraMemoryPos()[1]);
    _menu->cameraPosMemoryFlag = false;
    _menu->resetCameraMemoryPos();
  }
  if (_menu->getType() == "List") goToListItemRolling(static_cast<List*>(_menu));
  else if (_menu->getType() == "Tile") goToTileItem(_menu->selectIndex);

  this->render();
}
}
