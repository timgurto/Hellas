#include "Window.h"

#include <SDL_ttf.h>

#include <cassert>

#include "../Client.h"
#include "Button.h"
#include "ColorBlock.h"
#include "Element.h"
#include "Label.h"
#include "Line.h"
#include "ShadowBox.h"

px_t Window::HEADING_HEIGHT = 0;
px_t Window::CLOSE_BUTTON_SIZE = 0;

extern Renderer renderer;

void Window::onAddToClientWindowList(Client &client) { _client = &client; }

Window::Window(const ScreenPoint &mousePos)
    : _title(""),
      _dragging(false),
      _initFunction(nullptr),
      _isInitialized(true),
      _mousePos(mousePos) {
  hide();
  setLeftMouseUpFunction(&stopDragging);
  setMouseMoveFunction(&drag);
  addStructuralElements();
  setPreRefreshFunction(checkInitialized);
}

Window *Window::WithRectAndTitle(const ScreenRect &rect,
                                 const std::string &title,
                                 const ScreenPoint &mousePos) {
  auto window = new Window(mousePos);

  window->resize(rect.w, rect.h);
  window->setPosition(rect.x, rect.y);
  window->setTitle(title);

  return window;
}

Window *Window::InitializeLater(Client &client, InitFunction function,
                                const std::string &title) {
  auto window = new Window(client.mouse());
  window->setClient(client);

  window->setTitle(title);
  window->_initFunction = function;
  window->_isInitialized = false;

  return window;
}

void Window::addStructuralElements() {
  addBackground();
  addHeading();
  addBorder();
  addContent();
}

void Window::addBackground() {
  static const px_t UNINIT = 0;
  _background = new ColorBlock({1, 1, UNINIT, UNINIT});
  Element::addChild(_background);
}

void Window::addHeading() {
  static const px_t UNINIT = 0;
  _heading =
      new Label({0, 0, UNINIT, HEADING_HEIGHT}, _title, CENTER_JUSTIFIED);
  _heading->setLeftMouseDownFunction(&startDragging, this);
  Element::addChild(_heading);

  _headingLine = new Line({0, HEADING_HEIGHT}, UNINIT);
  _headingLine->setLeftMouseDownFunction(&startDragging, this);
  Element::addChild(_headingLine);

  _closeButton = new Button({UNINIT, 1, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE},
                            "", [this]() { hideWindow(this); });
  _closeButton->addChild(new Label({0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE},
                                   "x", CENTER_JUSTIFIED, CENTER_JUSTIFIED));
  Element::addChild(_closeButton);
}

void Window::addBorder() {
  static const px_t UNINIT = 0;
  _border = new ShadowBox({0, 0, UNINIT, UNINIT});
  Element::addChild(_border);
}

void Window::addContent() {
  static const px_t UNINIT = 0;
  _content = new Element({1, HEADING_HEIGHT + 2, UNINIT, UNINIT});
  Element::addChild(_content);
}

void Window::startDragging(Element &e, const ScreenPoint &) {
  Window &window = dynamic_cast<Window &>(e);
  window._dragOffset = window.client()->mouse() - ScreenPoint{window.rect()};
  window._dragging = true;
}

void Window::stopDragging(Element &e, const ScreenPoint &) {
  Window &window = dynamic_cast<Window &>(e);
  window._dragging = false;
}

void Window::drag(Element &e, const ScreenPoint &) {
  Window &window = dynamic_cast<Window &>(e);
  if (window._dragging)
    window.setPosition(
        toInt(window.client()->mouse().x - window._dragOffset.x),
        toInt(window.client()->mouse().y - window._dragOffset.y));
}

void Window::hideWindow(void *window) {
  Window &win = *static_cast<Window *>(window);
  win.hide();
}

void Window::addChild(Element *child) { _content->addChild(child); }

void Window::clearChildren() {
  _content->clearChildren();
  markChanged();
}

Element *Window::findChild(const std::string id) const {
  return _content->findChild(id);
}

void Window::resize(px_t w, px_t h) {  // TODO remove
  width(w);
  height(h);
}

void Window::width(px_t w) {
  const px_t windowWidth = w + 2,
             headingWidth = windowWidth - CLOSE_BUTTON_SIZE;

  Element::width(windowWidth);
  _content->width(w);
  _background->width(w);

  _heading->width(headingWidth);
  _headingLine->width(windowWidth);
  _closeButton->setPosition(headingWidth, 1);
  _border->width(windowWidth);
}

void Window::height(px_t h) {
  const px_t windowHeight = h + 2 + HEADING_HEIGHT;

  Element::height(windowHeight);
  _content->height(h);
  _background->height(windowHeight - 2);
  _border->height(windowHeight);
}

void Window::setTitle(const std::string &title) {
  _title = title;
  _heading->changeText(_title);
}

void Window::center() {
  auto x = (Client::SCREEN_X - contentWidth()) / 2,
       y = (Client::SCREEN_Y - contentHeight()) / 2;
  setPosition(x, y);
}

void Window::checkInitialized(Element &thisWindow) {
  Window &window = dynamic_cast<Window &>(thisWindow);
  if (window._isInitialized) return;
  assert(window._initFunction != nullptr);
  window._initFunction(*window.client());
  window._isInitialized = true;
}
