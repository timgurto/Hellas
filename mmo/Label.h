// (C) 2015 Tim Gurto

#ifndef LABEL_H
#define LABEL_H

#include <string>

#include "Element.h"

// Displays text.
class Label : public Element{
public:
    enum Justification{
        LEFT_JUSTIFIED,
        RIGHT_JUSTIFIED,
        CENTER_JUSTIFIED
    };

private:
    std::string _text;
    Justification _justification;

    virtual void refresh();

public:
    Label(const SDL_Rect &rect, const std::string &text,
          Justification justification = LEFT_JUSTIFIED);
};

#endif
