// (C) 2015 Tim Gurto

#ifndef LABEL_H
#define LABEL_H

#include <string>

#include "Element.h"

// Displays text.
class Label : public Element{
    std::string _text;
    Justification _justificationH, _justificationV;

    virtual void refresh();

public:
    Label(const SDL_Rect &rect, const std::string &text,
          Justification justificationH = LEFT_JUSTIFIED,
          Justification justificationV = TOP_JUSTIFIED);
};

#endif
