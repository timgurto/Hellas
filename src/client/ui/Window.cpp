
#include <SDL_ttf.h>

#include "Button.h"
#include "ColorBlock.h"
#include "Element.h"
#include "Label.h"
#include "Line.h"
#include "ShadowBox.h"
#include "Window.h"

px_t Window::HEADING_HEIGHT = 0;
px_t Window::CLOSE_BUTTON_SIZE = 0;

extern Renderer renderer;

Window::Window(const Rect &rect, const std::string &title):
Element(Rect(rect.x, rect.y, rect.w + 2, rect.h + 3 + HEADING_HEIGHT)),
_title(title),
_dragging(false),
_content(new Element(Rect(1, HEADING_HEIGHT + 2, rect.w, rect.h))){
    const Rect windowRect = this->rect();
    hide();
    setLeftMouseUpFunction(&stopDragging);
    setMouseMoveFunction(&drag);

    _background = new ColorBlock(Rect(1, 1, windowRect.w - 2, windowRect.h - 2));
    Element::addChild(_background);

    // Heading
    _heading = new Label(Rect(0, 0,  windowRect.w - CLOSE_BUTTON_SIZE, HEADING_HEIGHT), _title,
                         CENTER_JUSTIFIED);
    _heading->setLeftMouseDownFunction(&startDragging, this);
    Element::addChild(_heading);

    _headingLine = new Line(0, HEADING_HEIGHT, windowRect.w);
    _headingLine->setLeftMouseDownFunction(&startDragging, this);
    Element::addChild(_headingLine);

    _closeButton = new Button(Rect(windowRect.w - CLOSE_BUTTON_SIZE - 1, 1,
                                   CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE),
                              "", hideWindow, this);
    _closeButton->addChild(new Label(Rect(0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE), "x",
                                    CENTER_JUSTIFIED, CENTER_JUSTIFIED));
    Element::addChild(_closeButton);

    _border = new ShadowBox(Rect(0, 0, windowRect.w, windowRect.h));
    Element::addChild(_border);

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

Element *Window::findChild(const std::string id){
    return _content->findChild(id);
}

void Window::resize(px_t w, px_t h){ // TODO remove
    width(w);
    height(h);
}

void Window::width(px_t w){
    _content->width(w);
    _background->width(w);

    const px_t
        windowWidth = w + 2,
        headingWidth = windowWidth - CLOSE_BUTTON_SIZE;
    _heading->width(headingWidth);
    _headingLine->width(windowWidth);
    _closeButton->setPosition(headingWidth, 1);
    _border->width(windowWidth);
}

void Window::height(px_t h){
    _content->height(h);
    px_t windowHeight = h + 2 + HEADING_HEIGHT;
    _background->height(windowHeight - 2);
    _border->height(windowHeight);

}
