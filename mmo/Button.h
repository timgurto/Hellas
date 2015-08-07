// (C) 2015 Tim Gurto

#ifndef BUTTON_H
#define BUTTON_H

#include <string>

#include "Element.h"

// A button which can be clicked, showing visible feedback
class Button : public Element{
    std::string _caption;

    virtual void refresh();

public:
    Button(const SDL_Rect &rect, const std::string &caption = "");
};

#endif
