// (C) 2015 Tim Gurto

#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

#include "Element.h"

// A generic window for the in-game UI.
class Window : public Element{

    bool _visible; // If invisible, the window cannot be interacted with.
    std::string _title;

public:
    Window();
    Window(const SDL_Rect &rect, const std::string &title);

    void show() { _visible = true; }
    void hide() { _visible = false; }

    virtual void refresh() const;

    virtual void draw() const;
};

#endif
