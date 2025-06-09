#include "Button.h"

extern Renderer renderer;

Button::Button(const ScreenRect &rect, const std::string &caption,
               clickFun_t clickFunction)
    : Element(rect), _clickFun(clickFunction) {
  init(caption);
}

void Button::init(const std::string &caption) {
  Element::addChild(_background);
  Element::addChild(_content);

  _disabledMask->setAlpha(0xbf);
  _disabledMask->hide();
  Element::addChild(_disabledMask);

  Element::addChild(_border);

  if (!caption.empty()) {
    _caption = new Label({0, 0, rect().w, rect().h}, caption, CENTER_JUSTIFIED,
                         CENTER_JUSTIFIED);
    addChild(_caption);
  }

  width(rect().w);
  height(rect().h);

  setLeftMouseDownFunction(&mouseDown);
  setLeftMouseUpFunction(&mouseUp);
  setMouseMoveFunction(&mouseMove);
}

void Button::depress() {
  _border->setReversed(true);
  _content->setPosition(1, 1);  // Draw contents at an offset
  _depressed = true;
  markChanged();
}

void Button::release(bool click) {
  _border->setReversed(false);
  _content->setPosition(0, 0);
  if (click && _clickFun != nullptr) _clickFun();
  _depressed = false;
  markChanged();
}

void Button::mouseDown(Element &e, const ScreenPoint &) {
  Button &button = dynamic_cast<Button &>(e);
  if (!button._enabled) return;

  button._mouseButtonDown = true;
  button.depress();
}

void Button::mouseUp(Element &e, const ScreenPoint &mousePos) {
  Button &button = dynamic_cast<Button &>(e);
  if (!button._enabled) return;

  button._mouseButtonDown = false;
  if (button._depressed) {
    bool click =
        collision(mousePos, ScreenRect{0, 0, button.rect().w, button.rect().h});
    button.release(click);
  }
}

void Button::mouseMove(Element &e, const ScreenPoint &mousePos) {
  Button &button = dynamic_cast<Button &>(e);
  if (!button._enabled) return;

  if (collision(mousePos, ScreenRect{0, 0, button.rect().w, button.rect().h})) {
    if (button._mouseButtonDown && !button._depressed) button.depress();
  } else {
    if (button._depressed) button.release(false);
  }
}

void Button::addChild(Element *child) { _content->addChild(child); }

void Button::clearChildren() {
  _content->clearChildren();
  markChanged();
}

Element *Button::findChild(const std::string id) const {
  return _content->findChild(id);
}

void Button::width(px_t w) {
  Element::width(w);
  _background->width(w - 2);
  _content->width(w);
  _border->width(w);
  _disabledMask->width(w - 2);
  if (_caption != nullptr) _caption->width(w);
}

void Button::height(px_t h) {
  Element::height(h);
  _background->height(h - 2);
  _content->height(h);
  _border->height(h);
  _disabledMask->height(h - 2);
  if (_caption != nullptr) _caption->height(h);
}

void Button::enable() {
  _enabled = true;
  _disabledMask->hide();
}

void Button::disable() {
  _enabled = false;
  _disabledMask->show();
}

void Button::setEnabled(bool enabled) {
  if (enabled)
    enable();
  else
    disable();
}
