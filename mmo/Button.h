// (C) 2015 Tim Gurto

#ifndef BUTTON_H
#define BUTTON_H

#include <string>

#include "Element.h"
#include "Point.h"
#include "ShadowBox.h"

// A button which can be clicked, showing visible feedback and performing a function.
class Button : public Element{
public:
    typedef void (*clickFun_t)(void *data);

private:
    Element *_content;

    clickFun_t _clickFun;
    void *_clickData; // Data passed to _clickFun().

    bool _mouseButtonDown;
    bool _depressed;

    void depress();
    void release(bool click); // click: whether, on release, the button's _clickFun will be called.

    static void mouseDown(Element &e);
    static void mouseUp(Element &e, const Point &mousePos);
    static void mouseMove(Element &e, const Point &mousePos);

    virtual void refresh();

public:
    Button(const SDL_Rect &rect, const std::string &caption = "", clickFun_t clickFunction = 0,
           void *clickData = 0);
    virtual void addChild(Element *child);
};

#endif
