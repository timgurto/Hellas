#include <cassert>

#include <SDL_ttf.h>

#include "Button.h"
#include "ColorBlock.h"
#include "Element.h"
#include "Label.h"
#include "Line.h"
#include "ShadowBox.h"
#include "Window.h"
#include "../Client.h"

px_t Window::HEADING_HEIGHT = 0;
px_t Window::CLOSE_BUTTON_SIZE = 0;

extern Renderer renderer;

Window::Window():
_title(""),
_dragging(false),
_initFunction(nullptr),
_isInitialized(true)
{
    hide();
    setLeftMouseUpFunction(&stopDragging);
    setMouseMoveFunction(&drag);
    addStructuralElements();
    setPreRefreshFunction(checkInitialized);
}

Window *Window::WithRectAndTitle(const Rect &rect, const std::string &title){
    auto window = new Window;

    window->resize(rect.w, rect.h);
    window->setPosition(rect.x, rect.y);
    window->setTitle(title);

    return window;
}

Window *Window::InitializeLater(InitFunction function){
    auto window = new Window;

    window->_initFunction = function;
    window->_isInitialized = false;

    return window;
}

void Window::addStructuralElements(){
    addBackground();
    addHeading();
    addBorder();
    addContent();
}

void Window::addBackground(){
    static const px_t UNINIT = 0;
    _background = new ColorBlock(Rect(1, 1, UNINIT, UNINIT));
    Element::addChild(_background);
}

void Window::addHeading(){
    static const px_t UNINIT = 0;
    _heading = new Label(Rect(0, 0,  UNINIT, HEADING_HEIGHT), _title, CENTER_JUSTIFIED);
    _heading->setLeftMouseDownFunction(&startDragging, this);
    Element::addChild(_heading);

    _headingLine = new Line(0, HEADING_HEIGHT, UNINIT);
    _headingLine->setLeftMouseDownFunction(&startDragging, this);
    Element::addChild(_headingLine);

    _closeButton = new Button(Rect(UNINIT, 1, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE),
                                "", hideWindow, this);
    _closeButton->addChild(new Label(Rect(0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE), "x",
                                    CENTER_JUSTIFIED, CENTER_JUSTIFIED));
    Element::addChild(_closeButton);
}

void Window::addBorder(){
    static const px_t UNINIT = 0;
    _border = new ShadowBox(Rect(0, 0, UNINIT, UNINIT));
    Element::addChild(_border);
}

void Window::addContent(){
    static const px_t UNINIT = 0;
    _content = new Element(Rect(1, HEADING_HEIGHT + 2, UNINIT, UNINIT));
    Element::addChild(_content);
}


void Window::startDragging(Element &e, const Point &mousePos){
    Window &window = dynamic_cast<Window &>(e);
    window._dragOffset = *absMouse - window.rect();
    window._dragging = true;
}

void Window::stopDragging(Element &e, const Point &mousePos){
    Window &window = dynamic_cast<Window &>(e);
    window._dragging = false;
}

void Window::drag(Element &e, const Point &mousePos){
    Window &window = dynamic_cast<Window &>(e);
    if (window._dragging) 
        window.setPosition(toInt(absMouse->x - window._dragOffset.x),
                           toInt(absMouse->y - window._dragOffset.y));
}

void Window::hideWindow(void *window){
    Window &win = * static_cast<Window *>(window);
    win.hide();
}

void Window::addChild(Element *child){
    _content->addChild(child);
}

void Window::clearChildren(){
    _content->clearChildren();
    markChanged();
}

Element *Window::findChild(const std::string id) const{
    return _content->findChild(id);
}

void Window::resize(px_t w, px_t h){ // TODO remove
    width(w);
    height(h);
}

void Window::width(px_t w){
    const px_t
        windowWidth = w + 2,
        headingWidth = windowWidth - CLOSE_BUTTON_SIZE;

    Element::width(windowWidth);
    _content->width(w);
    _background->width(w);

    _heading->width(headingWidth);
    _headingLine->width(windowWidth);
    _closeButton->setPosition(headingWidth, 1);
    _border->width(windowWidth);
}

void Window::height(px_t h){
    const px_t windowHeight = h + 2 + HEADING_HEIGHT;

    Element::height(windowHeight);
    _content->height(h);
    _background->height(windowHeight - 2);
    _border->height(windowHeight);
}

void Window::setTitle(const std::string &title){
    _title = title;
    _heading->changeText(_title);
}

void Window::checkInitialized(Element &thisWindow){
    Window &window = dynamic_cast<Window &>(thisWindow);
    if (window._isInitialized)
        return;
    assert(window._initFunction != nullptr);
    window._initFunction(*Client::_instance);
    window._isInitialized = true;
}
