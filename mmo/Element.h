// (C) 2015 Tim Gurto

#include <SDL.h>
#include <vector>

#include "Color.h"
#include "Texture.h"

#ifndef ELEMENT_H
#define ELEMENT_H


/*
A UI element, making up part of a Window.
The base class may be used as an invisible container for other Elements, to improve the efficiency
of collision detection.
*/
class Element{
    std::vector<Element> _children;

protected:
    static const Color
        BACKGROUND_COLOR,
        FONT_COLOR;
    static TTF_Font *_font;

    SDL_Rect _rect; // Location and dimensions within window
    Texture _texture; // A memoized image of the element, redrawn only when necessary.

    void drawChildren() const;

public:
    Element();
    Element(const SDL_Rect &rect);

    static TTF_Font *font() { return _font; }
    static void font(TTF_Font *newFont) { _font = newFont; }

    void addElement(const Element &element) { _children.push_back(element); };

    // Redraw the Element to its texture, usually after something has changed.
    // Should be called after initialization.
    virtual void refresh() const;

    virtual void draw() const; // Draw the existing texture to its designated location.
};

#endif
