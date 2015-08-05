// (C) 2015 Tim Gurto

#include "Renderer.h"
#include "Window.h"

const Color Window::BACKGROUND_COLOR = Color::GREY_4;

extern Renderer renderer;

Window::Window(const SDL_Rect &rect):
_rect(rect),
_texture(rect.w, rect.h){
    _texture.setRenderTarget();
    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.fill();
    renderer.setRenderTarget();
}

void Window::draw() const{
    _texture.draw(_rect);
}