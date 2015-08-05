// (C) 2015 Tim Gurto

#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

#include "Texture.h"

// A generic window for the in-game UI.
class Window{
    static const Color BACKGROUND_COLOR;

    SDL_Rect _rect; // The window's location and dimensions

    Texture _texture;

public:
    Window(const SDL_Rect &rect);

    void draw() const;
};

#endif