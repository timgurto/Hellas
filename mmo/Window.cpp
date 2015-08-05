// (C) 2015 Tim Gurto

#include <SDL_ttf.h>

#include "Renderer.h"
#include "Window.h"

const Color Window::BACKGROUND_COLOR = Color::GREY_4;
const Color Window::FONT_COLOR = Color::WHITE;
TTF_Font *Window::_font = 0;

extern Renderer renderer;

Window::Window(){}

Window::Window(const SDL_Rect &rect, const std::string &title):
_rect(rect),
_texture(rect.w, rect.h){
    _texture.setRenderTarget();

    // Draw background
    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.fill();

    // Draw title
    Texture titleTexture(_font, title, FONT_COLOR);
    titleTexture.draw((rect.w - titleTexture.width()) / 2, 0);

    renderer.setRenderTarget();
}

void Window::draw() const{
    _texture.draw(_rect);
}