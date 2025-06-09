#include "CheckBox.h"

#include "ColorBlock.h"
#include "Label.h"

extern Renderer renderer;

const px_t CheckBox::BOX_SIZE = 8;
const px_t CheckBox::GAP = 3;
const px_t CheckBox::Y_OFFSET = 1;

CheckBox::CheckBox(Client &client, const ScreenRect &rect, bool &linkedBool,
                   const std::string &caption, bool inverse)
    : Element(rect),
      _client(client),
      _linkedBool(linkedBool),
      _inverse(inverse),
      _mouseButtonDown(false),
      _depressed(false) {
  if (!caption.empty())
    addChild(new Label({BOX_SIZE + GAP, 0, rect.w, rect.h}, caption,
                       LEFT_JUSTIFIED, CENTER_JUSTIFIED));
  setLeftMouseDownFunction(&mouseDown);
  setLeftMouseUpFunction(&mouseUp);
  setMouseMoveFunction(&mouseMove);
}

void CheckBox::toggleBool() {
  _linkedBool = !_linkedBool;
  _lastCheckedValue = _linkedBool;
}

void CheckBox::depress() {
  _depressed = true;
  if (_onChangeFunction) _onChangeFunction(_client);
  markChanged();
}

void CheckBox::release(bool click) {
  if (click) toggleBool();
  markChanged();
  _depressed = false;
  if (_onChangeFunction) _onChangeFunction(_client);
}

void CheckBox::mouseDown(Element &e, const ScreenPoint &) {
  CheckBox &checkBox = dynamic_cast<CheckBox &>(e);
  checkBox._mouseButtonDown = true;
  checkBox.depress();
}

void CheckBox::mouseUp(Element &e, const ScreenPoint &mousePos) {
  CheckBox &checkBox = dynamic_cast<CheckBox &>(e);
  checkBox._mouseButtonDown = false;
  if (checkBox._depressed) {
    bool click = collision(
        mousePos, ScreenRect{0, 0, checkBox.rect().w, checkBox.rect().h});
    checkBox.release(click);
  }
}

void CheckBox::mouseMove(Element &e, const ScreenPoint &mousePos) {
  CheckBox &checkBox = dynamic_cast<CheckBox &>(e);
  if (collision<px_t>(mousePos, {0, 0, checkBox.rect().w, checkBox.rect().h})) {
    if (checkBox._mouseButtonDown && !checkBox._depressed) checkBox.depress();
  } else {
    if (checkBox._depressed) checkBox.release(false);
  }
}

void CheckBox::checkIfChanged() {
  if (_lastCheckedValue != _linkedBool) {
    _lastCheckedValue = _linkedBool;
    markChanged();
  }
  Element::checkIfChanged();
}

void CheckBox::refresh() {
  // Box
  const auto boxRect =
      ScreenRect{0, (rect().h - BOX_SIZE) / 2 + Y_OFFSET, BOX_SIZE, BOX_SIZE};
  renderer.setDrawColor(FONT_COLOR);
  renderer.drawRect(boxRect);

  // Check (filled square)
  if (_depressed || (_linkedBool ^ _inverse)) {
    if (_depressed) renderer.setDrawColor(SHADOW_LIGHT);
    static const auto CHECK_OFFSET_RECT = ScreenRect{2, 2, -4, -4};
    renderer.fillRect(boxRect + CHECK_OFFSET_RECT);
  }
}
