// (C) 2015 Tim Gurto

#include <SDL_ttf.h>

#include "Renderer.h"
#include "Window.h"

extern Renderer renderer;

Window::Window():
_visible(false){}

Window::Window(const SDL_Rect &rect, const std::string &title):
Element(rect),
_title(title),
_visible(false){}

void Window::refresh() const{
    _texture.setRenderTarget();

    // Draw background
    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.fill();

    // Draw title
    Texture titleTexture(_font, _title, FONT_COLOR);
    titleTexture.draw((_rect.w - titleTexture.width()) / 2, 0);

    // Draw children
    drawChildren();

    renderer.setRenderTarget();
}

void Window::draw() const{
    if (_visible)
        Element::draw();
}
