// (C) 2015 Tim Gurto

#ifndef LABEL_H
#define LABEL_H

#include <string>

#include "Element.h"

// Displays text.
class Label : public Element{
    std::string _text;
    Justification _justification;

    virtual void refresh();

public:
    Label(const SDL_Rect &rect, const std::string &text,
          Justification justification = LEFT_JUSTIFIED);
};

#endif
