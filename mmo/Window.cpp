// (C) 2015 Tim Gurto

#include <SDL_ttf.h>

#include "Label.h"
#include "Window.h"

const int Window::HEADING_HEIGHT = 12;

extern Renderer renderer;

Window::Window(const SDL_Rect &rect, const std::string &title):
Element(rect),
_title(title),
_visible(false),
_dragging(false){
    addChild(new Label(makeRect(0, 0, rect.w, HEADING_HEIGHT), _title, Label::CENTER_JUSTIFIED));
}

void Window::onMouseDown(const Point &mousePos){
    if (collision(mousePos, makeRect(rect().x, rect().y, rect().w, HEADING_HEIGHT))) {
        _dragOffset = mousePos - rect();
        _dragging = true;
    }
    Element::onMouseDown(mousePos);
}

void Window::onMouseUp(const Point &mousePos){
    _dragging = false;
    Element::onMouseUp(mousePos);
}

void Window::onMouseMove(const Point &mousePos){
    if (_dragging) 
        location(static_cast<int>(mousePos.x - _dragOffset.x + .5),
                 static_cast<int>(mousePos.y - _dragOffset.y + .5));
    Element::onMouseMove(mousePos);
}

void Window::refresh(){
    renderer.pushRenderTarget(_texture);

    // Draw background
    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.fill();

    // Draw children
    drawChildren();

    renderer.popRenderTarget();
}

void Window::draw(){
    if (_visible)
        Element::draw();
}
