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
    static TTF_Font *_font;

    mutable bool _changed; // If true, this element should be refreshed before next being drawn.

    SDL_Rect _rect; // Location and dimensions within window

    // Redraw the Element to its texture, usually after something has changed.
    virtual void refresh() const;

protected:
    static const Color
        BACKGROUND_COLOR,
        FONT_COLOR;

    Texture _texture; // A memoized image of the element, redrawn only when necessary.

    static void font(TTF_Font *newFont) { _font = newFont; }
    const SDL_Rect &rect() const { return _rect; }

    void drawChildren() const;

public:
    Element();
    Element(const SDL_Rect &rect);

    static TTF_Font *font() { return _font; }

    void addElement(const Element &element) { _children.push_back(element); };

    virtual void draw() const; // Draw the existing texture to its designated location.
};

#endif
