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

    std::string _title;
    bool _dragging; // Whether this window is currently being dragged by the mouse.
    Point _dragOffset; // While dragging, where the mouse is on the window.

    virtual void refresh();

public:
    Window(const SDL_Rect &rect, const std::string &title);

    static void hideWindow(void *window);

    static void startDragging(Element &e);
    static void stopDragging(Element &e, const Point &mousePos);
    static void drag(Element &e, const Point &mousePos);
};

#endif
