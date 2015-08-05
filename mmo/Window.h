// (C) 2015 Tim Gurto

#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

#include "Texture.h"

// A generic window for the in-game UI.
class Window{
    static const Color
        BACKGROUND_COLOR,
        FONT_COLOR;
    static TTF_Font *_font;

    SDL_Rect _rect; // The window's location and dimensions

    Texture _texture;

public:
    Window();
    Window(const SDL_Rect &rect, const std::string &title);

    static TTF_Font *font() { return _font; }
    static void font(TTF_Font *newFont) { _font = newFont; }

    void draw() const;
};

#endif