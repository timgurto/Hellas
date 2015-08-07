// (C) 2015 Tim Gurto

#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

#include "Element.h"

// A generic window for the in-game UI.
class Window : public Element{

    static const int HEADING_HEIGHT;
    static const int CLOSE_BUTTON_SIZE;

    bool _visible; // If invisible, the window cannot be interacted with.
    std::string _title;
    bool _dragging; // Whether this window is currently being dragged by the mouse.
    Point _dragOffset; // While dragging, where the mouse is on the window.

    virtual void refresh();

public:
    Window(const SDL_Rect &rect, const std::string &title);

    void show() { _visible = true; }
    void hide() { _visible = false; }

    static void startDragging(Element &e);
    static void stopDragging(Element &e);
    static void drag(Element &e);

    virtual void draw();
};

#endif
