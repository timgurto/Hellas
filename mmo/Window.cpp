// (C) 2015 Tim Gurto

#include <SDL_ttf.h>

#include "Button.h"
#include "Label.h"
#include "Line.h"
#include "ShadowBox.h"
#include "Window.h"

const int Window::HEADING_HEIGHT = 12;
const int Window::CLOSE_BUTTON_SIZE = 11;

extern Renderer renderer;

Window::Window(const SDL_Rect &rect, const std::string &title):
Element(makeRect(rect.x, rect.y, rect.w + 2, rect.h + 2 + HEADING_HEIGHT)),
_title(title),
_dragging(false),
_content(new Element(makeRect(1, HEADING_HEIGHT + 1, rect.w, rect.h))){
    const SDL_Rect windowRect = this->rect();
    hide();
    setMouseUpFunction(&stopDragging);
    setMouseMoveFunction(&drag);
    fillBackground();

    // Heading
    Label *const heading = new Label(makeRect(0, 0, windowRect.w - CLOSE_BUTTON_SIZE, HEADING_HEIGHT),
                                     _title, CENTER_JUSTIFIED);
    heading->setMouseDownFunction(&startDragging, this);
    Element::addChild(heading);

    Line *const headingLine = new Line(0, HEADING_HEIGHT, windowRect.w);
    headingLine->setMouseDownFunction(&startDragging, this);
    Element::addChild(headingLine);

    Button *const closeButton = new Button(makeRect(windowRect.w - CLOSE_BUTTON_SIZE - 1, 1,
                                                    CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE), "",
                                                    hideWindow, this);
    Label *const closeButtonLabel = new Label(makeRect(0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE),
                                              "x", CENTER_JUSTIFIED, BOTTOM_JUSTIFIED);
    closeButton->addChild(closeButtonLabel);
    Element::addChild(closeButton);

    Element::addChild(new ShadowBox(makeRect(0, 0, windowRect.w, windowRect.h)));

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
        window.rect(toInt(absMouse->x - window._dragOffset.x),
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
