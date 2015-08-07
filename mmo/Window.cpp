// (C) 2015 Tim Gurto

#include <SDL_ttf.h>

#include "Label.h"
#include "Window.h"

const int Window::HEADING_HEIGHT = 12;

extern Renderer renderer;

Window::Window(const SDL_Rect &rect, const std::string &title):
Element(rect),
_title(title),
_visible(false){
    addChild(new Label(makeRect(0, 0, rect.w, HEADING_HEIGHT), _title, Label::CENTER_JUSTIFIED));
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
