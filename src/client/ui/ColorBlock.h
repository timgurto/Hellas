// (C) 2015 Tim Gurto

#ifndef COLOR_BLOCK_H
#define COLOR_BLOCK_H

#include "Element.h"
#include "../../Color.h"

// A colored, filled rectangle
class ColorBlock : public Element {
    Color _color;

public:
    ColorBlock(const SDL_Rect &rect, const Color &color = BACKGROUND_COLOR);

    virtual void refresh();
};

#endif
