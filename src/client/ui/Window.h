// (C) 2015 Tim Gurto

#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

#include "Element.h"

// A generic window for the in-game UI.
class Window : public Element{
    std::string _title;
    bool _dragging; // Whether this window is currently being dragged by the mouse.
    Point _dragOffset; // While dragging, where the mouse is on the window.
    Element *_content;

public:
    static int HEADING_HEIGHT;
    static int CLOSE_BUTTON_SIZE;

    Window(const Rect &rect, const std::string &title);

    static void hideWindow(void *window);

    static void startDragging(Element &e, const Point &mousePos);
    static void stopDragging(Element &e, const Point &mousePos);
    static void drag(Element &e, const Point &mousePos);

    virtual void addChild(Element *child);
    virtual void clearChildren();
    virtual Element *findChild(const std::string id);
};

#endif
