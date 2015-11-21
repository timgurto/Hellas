// (C) 2015 Tim Gurto

#ifndef LABEL_H
#define LABEL_H

#include <string>

#include "Element.h"

class Color;

// Displays text.
class Label : public Element{
    std::string _text;
    Justification _justificationH, _justificationV;
    bool _matchWidth;
    Color _color;

public:
    Label(const Rect &rect, const std::string &text,
          Justification justificationH = LEFT_JUSTIFIED,
          Justification justificationV = TOP_JUSTIFIED);

    virtual void refresh() override;

    void setColor(const Color &color);

    void matchW(); // Set the label's width to the width of the contained text image.
};

#endif
