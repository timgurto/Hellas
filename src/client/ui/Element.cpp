#include "Element.h"

#include <cassert>

#include "../../WorkerThread.h"
#include "../../util.h"
#include "../Client.h"
#include "../Tooltip.h"
#include "Window.h"

extern Renderer renderer;
extern WorkerThread SDLThread;

Color Element::BACKGROUND_COLOR;
Color Element::SHADOW_LIGHT;
Color Element::SHADOW_DARK;
Color Element::FONT_COLOR;

TTF_Font *Element::_font = nullptr;
px_t Element::textOffset = 0;
px_t Element::TEXT_HEIGHT = 0;
px_t Element::ITEM_HEIGHT = 0;

Texture Element::transparentBackground;

const Element *Element::_currentTooltipElement = nullptr;

bool Element::initialized = false;

Element::Element(const ScreenRect &rect) : _rect(rect) {
  assert(initialized);
  _texture = {_rect.w, _rect.h};
  _texture.setBlend(SDL_BLENDMODE_BLEND);
}

Element::~Element() {
  if (_currentTooltipElement == this) _currentTooltipElement = nullptr;
  clearChildren();
}

void Element::initialize() {
  if (initialized) return;

  BACKGROUND_COLOR = Color::WINDOW_BACKGROUND;
  SHADOW_LIGHT = Color::WINDOW_LIGHT;
  SHADOW_DARK = Color::WINDOW_DARK;
  FONT_COLOR = Color::WINDOW_FONT;

  ITEM_HEIGHT = max(Client::ICON_SIZE, TEXT_HEIGHT);
  Window::HEADING_HEIGHT = TEXT_HEIGHT + 3;
  Window::CLOSE_BUTTON_SIZE = TEXT_HEIGHT + 2;

  initialized = true;
}

void Element::rect(px_t x, px_t y, px_t w, px_t h) {
  setPosition(x, y);
  width(w);
  height(h);
  _dimensionsChanged = true;
}

void Element::rect(const ScreenRect &rhs) {
  setPosition(rhs.x, rhs.y);
  width(rhs.w);
  height(rhs.h);
  _dimensionsChanged = true;
}

void Element::setPosition(px_t x, px_t y) {
  _rect.x = x;
  _rect.y = y;
  if (_parent != nullptr) _parent->markChanged();
}

void Element::width(px_t w) {
  _rect.w = w;
  _dimensionsChanged = true;
}

void Element::height(px_t h) {
  _rect.h = h;
  _dimensionsChanged = true;
}

ScreenRect Element::rectToFill() { return {0, 0, _rect.w, _rect.h}; }

void Element::markChanged() const {
  _changed = true;
  if (_parent != nullptr) _parent->markChanged();
}

void Element::show() {
  if (!_visible && _parent != nullptr) _parent->markChanged();
  _visible = true;
}

void Element::hide() {
  if (_visible && _parent != nullptr) _parent->markChanged();
  _visible = false;
}

void Element::toggleVisibility() {
  _visible = !_visible;
  if (_parent != nullptr) _parent->markChanged();
}

void Element::drawChildren() const {
  for (Element *child : _children) {
    if (child->_changed) markChanged();

    child->draw();
  }
}

void Element::checkIfChanged() {
  for (Element *child : _children) {
    if (!child->visible()) continue;
    child->checkIfChanged();
  }
}

bool Element::onLeftMouseDown(const ScreenPoint &mousePos) {
  // Assumption: if this is called, then the mouse collides with the element.
  // Assumption: each element has at most one child that collides with the
  // mouse.
  if (!_visible) return false;
  if (_ignoreMouseEvents) return false;
  const ScreenPoint relativeLocation = mousePos - position();
  bool functionCalled = false;
  for (Element *child : _children) {
    if (collision(relativeLocation, child->rect())) {
      if (child->onLeftMouseDown(relativeLocation)) {
        functionCalled = true;
      }
    }
  }
  if (functionCalled) return true;
  /*
  If execution gets here, then this element has no children that both collide
  and have _mouseDown defined.
  */
  if (_leftMouseDown != nullptr) {
    _leftMouseDown(*_leftMouseDownElement, relativeLocation);
    return true;
  } else
    return false;
}

bool Element::onRightMouseDown(const ScreenPoint &mousePos) {
  // Assumption: if this is called, then the mouse collides with the element.
  // Assumption: each element has at most one child that collides with the
  // mouse.
  if (!_visible) return false;
  if (_ignoreMouseEvents) return false;
  const ScreenPoint relativeLocation = mousePos - position();
  bool functionCalled = false;
  for (Element *child : _children) {
    if (collision(relativeLocation, child->rect())) {
      if (child->onRightMouseDown(relativeLocation)) {
        functionCalled = true;
      }
    }
  }
  if (functionCalled) return true;
  /*
  If execution gets here, then this element has no children that both collide
  and have _mouseDown defined.
  */
  if (_rightMouseDown != nullptr) {
    _rightMouseDown(*_rightMouseDownElement, relativeLocation);
    return true;
  } else
    return false;
}

bool Element::onScrollUp(const ScreenPoint &mousePos) {
  // Assumption: if this is called, then the mouse collides with the element.
  // Assumption: each element has at most one child that collides with the
  // mouse.
  if (!_visible) return false;
  if (_ignoreMouseEvents) return false;
  const ScreenPoint relativeLocation = mousePos - position();
  bool functionCalled = false;
  for (Element *child : _children) {
    if (collision(relativeLocation, child->rect())) {
      if (child->onScrollUp(relativeLocation)) {
        functionCalled = true;
      }
    }
  }
  if (functionCalled) return true;
  /*
  If execution gets here, then this element has no children that both collide
  and have _scrollUp defined.
  */
  if (_scrollUp != nullptr) {
    _scrollUp(*_scrollUpElement);
    return true;
  } else
    return false;
}

bool Element::onScrollDown(const ScreenPoint &mousePos) {
  // Assumption: if this is called, then the mouse collides with the element.
  // Assumption: each element has at most one child that collides with the
  // mouse.
  if (!_visible) return false;
  if (_ignoreMouseEvents) return false;
  const ScreenPoint relativeLocation = mousePos - position();
  bool functionCalled = false;
  for (Element *child : _children) {
    if (collision(relativeLocation, child->rect())) {
      if (child->onScrollDown(relativeLocation)) {
        functionCalled = true;
      }
    }
  }
  if (functionCalled) return true;
  /*
  If execution gets here, then this element has no children that both collide
  and have _scrollDown defined.
  */
  if (_scrollDown != nullptr) {
    _scrollDown(*_scrollDownElement);
    return true;
  } else
    return false;
}

bool Element::onLeftMouseUp(const ScreenPoint &mousePos) {
  if (!_visible) return false;
  if (_ignoreMouseEvents) return false;
  const ScreenPoint relativeLocation = mousePos - position();
  auto ret = false;
  if (_leftMouseUp != nullptr) {
    _leftMouseUp(*_leftMouseUpElement, relativeLocation);
    ret = true;
  }
  for (Element *child : _children) {
    auto mouseEventOnChild = child->onLeftMouseUp(relativeLocation);
    ret = ret || mouseEventOnChild;
  }
  return ret;
}

bool Element::onRightMouseUp(const ScreenPoint &mousePos) {
  if (!_visible) return false;
  if (_ignoreMouseEvents) return false;
  const ScreenPoint relativeLocation = mousePos - position();
  auto ret = false;
  if (_rightMouseUp != nullptr) {
    _rightMouseUp(*_rightMouseUpElement, relativeLocation);
    ret = true;
  }
  for (Element *child : _children) {
    auto mouseEventOnChild = child->onRightMouseUp(relativeLocation);
    ret = ret || mouseEventOnChild;
  }
  return ret;
}

void Element::onMouseMove(const ScreenPoint &mousePos) {
  if (!_visible) return;
  const ScreenPoint relativeLocation = mousePos - position();
  if (_mouseMove != nullptr) _mouseMove(*_mouseMoveElement, relativeLocation);
  if (_tooltip && collision(mousePos, rect())) _currentTooltipElement = this;
  for (Element *child : _children) child->onMouseMove(relativeLocation);
}

void Element::setLeftMouseDownFunction(mouseDownFunction_t f, Element *e) {
  _leftMouseDown = f;
  _leftMouseDownElement = e ? e : this;
}

void Element::setLeftMouseUpFunction(mouseUpFunction_t f, Element *e) {
  _leftMouseUp = f;
  _leftMouseUpElement = e ? e : this;
}

void Element::setRightMouseDownFunction(mouseDownFunction_t f, Element *e) {
  _rightMouseDown = f;
  _rightMouseDownElement = e ? e : this;
}

void Element::setRightMouseUpFunction(mouseUpFunction_t f, Element *e) {
  _rightMouseUp = f;
  _rightMouseUpElement = e ? e : this;
}

void Element::setMouseMoveFunction(mouseMoveFunction_t f, Element *e) {
  _mouseMove = f;
  _mouseMoveElement = e ? e : this;
}

void Element::setScrollUpFunction(scrollUpFunction_t f, Element *e) {
  _scrollUp = f;
  _scrollUpElement = e ? e : this;
}

void Element::setScrollDownFunction(scrollDownFunction_t f, Element *e) {
  _scrollDown = f;
  _scrollDownElement = e ? e : this;
}

void Element::setPreRefreshFunction(preRefreshFunction_t f, Element *e) {
  _preRefresh = f;
  _preRefreshElement = e ? e : this;
}

// Note: takes ownership of child.  Child should be allocated with new.
void Element::addChild(Element *child) {
  assert(child);
  _children.push_back(child);
  child->_parent = this;
  markChanged();
}

void Element::clearChildren() {
  for (Element *child : _children) delete child;
  _children.clear();
  markChanged();
}

Element *Element::findChild(const std::string id) const {
  // Check this level first
  for (Element *child : _children)
    if (child->_id == id) return child;

  // Check children recursively
  for (Element *child : _children) {
    Element *found = child->findChild(id);
    if (found) return found;
  }
  return nullptr;
}

void Element::resetTooltip() { _currentTooltipElement = nullptr; }

void Element::setTooltip(const std::string &text) {
  _localCopyOfTooltip = Tooltip::basicTooltip(text);
  _tooltip = &_localCopyOfTooltip;
}

void Element::setTooltip(const Tooltip &tooltip) {
  _localCopyOfTooltip = tooltip;
  _tooltip = &_localCopyOfTooltip;
}

void Element::clearTooltip() { _tooltip = nullptr; }

const Tooltip *Element::tooltip() {
  if (!_currentTooltipElement) return nullptr;
  return _currentTooltipElement->_tooltip;
}

void Element::draw() {
  if (!_visible) return;
  if (_parent == nullptr) checkIfChanged();
  if (_dimensionsChanged) {
    _texture = Texture(_rect.w, _rect.h);
    markChanged();
    _dimensionsChanged = false;
  }
  if (!_texture) markChanged();
  if (_changed) {
    if (_preRefresh != nullptr) _preRefresh(*_preRefreshElement);
    if (!_texture) return;

    renderer.pushRenderTarget(_texture);
    makeBackgroundTransparent();
    refresh();
    drawChildren();
    renderer.popRenderTarget();
    if (_alpha != SDL_ALPHA_OPAQUE) _texture.setAlpha(_alpha);
    _changed = false;
  }
  _texture.setBlend(SDL_BLENDMODE_BLEND);
  _texture.draw(_rect);
}

void Element::forceRefresh() {
  if (_parent == nullptr) transparentBackground = {};
  _changed = true;
  for (Element *child : _children) child->forceRefresh();
}

void Element::makeBackgroundTransparent() {
  if (!transparentBackground) createTransparentBackground();

  auto dstRect = ScreenRect(0, 0, _rect.w, _rect.h);
  transparentBackground.draw(dstRect);
}

void Element::createTransparentBackground() {
  transparentBackground =
      Texture(4, 4);  // Texture(renderer.width(), renderer.height());
  transparentBackground.setAlpha(0);
  transparentBackground.setBlend(SDL_BLENDMODE_NONE);
}

void Element::toggleVisibilityOf(void *element) {
  Element *e = static_cast<Element *>(element);
  e->toggleVisibility();
}

void Element::setAlpha(Uint8 alpha) {
  // Currently broken; behaves strangely on some window events.
  _alpha = alpha;
  markChanged();
}
