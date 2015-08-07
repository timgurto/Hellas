// (C) 2015 Tim Gurto

#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

#include "Element.h"

// A generic window for the in-game UI.
class Window : public Element{

    static const int HEADING_HEIGHT;

    bool _visible; // If invisible, the window cannot be interacted with.
    std::string _title;

    virtual void refresh();

public:
    Window(const SDL_Rect &rect, const std::string &title);

    void show() { _visible = true; }
    void hide() { _visible = false; }

    virtual void draw();
};

#endif
