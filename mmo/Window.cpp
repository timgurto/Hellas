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
Element(rect),
_title(title),
_dragging(false){
    hide();
    setMouseUpFunction(&stopDragging);
    setMouseMoveFunction(&drag);
    fillBackground();

    // Heading
    Label *heading = new Label(makeRect(0, 0, rect.w - CLOSE_BUTTON_SIZE, HEADING_HEIGHT),
                               _title, CENTER_JUSTIFIED);
    heading->setMouseDownFunction(&startDragging, this);
    addChild(heading);

    Line *headingLine = new Line(0, HEADING_HEIGHT, rect.w);
    headingLine->setMouseDownFunction(&startDragging, this);
    addChild(headingLine);

    Button *closeButton = new Button(makeRect(rect.w - CLOSE_BUTTON_SIZE - 1, 1,
                                              CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE), "",
                                              hideWindow, this);
    Label *closeButtonLabel = new Label(makeRect(0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE),
                                        "x", CENTER_JUSTIFIED, BOTTOM_JUSTIFIED);
    closeButton->addChild(closeButtonLabel);
    addChild(closeButton);

    addChild(new ShadowBox(makeRect(0, 0, rect.w, rect.h)));
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
