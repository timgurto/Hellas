// (C) 2015 Tim Gurto

#include <SDL.h>
#include <vector>

#include "Color.h"
#include "Renderer.h"
#include "Texture.h"

#ifndef ELEMENT_H
#define ELEMENT_H


/*
A UI element, making up part of a Window.
The base class may be used as an invisible container for other Elements, to improve the efficiency
of collision detection.
*/
class Element{
public:
    enum Justification{
        LEFT_JUSTIFIED,
        RIGHT_JUSTIFIED,
        CENTER_JUSTIFIED
    };
    enum Orientation{
        HORIZONTAL,
        VERTICAL
    };

private:
    std::vector<Element*> _children;
    static TTF_Font *_font;

    static Texture transparentBackground;

    mutable bool _changed; // If true, this element should be refreshed before next being drawn.

    SDL_Rect _rect; // Location and dimensions within window

    // Redraw the Element to its texture, usually after something has changed.
    virtual void refresh();

protected:
    static const Color
        BACKGROUND_COLOR,
        SHADOW_LIGHT,
        SHADOW_DARK,
        FONT_COLOR;
    Texture _texture; // A memoized image of the element, redrawn only when necessary.
    const SDL_Rect &rect() const { return _rect; }
    void rect(int x, int y) { _rect.x = x; _rect.y = y; }
    void rect(int x, int y, int w, int h) { _rect = makeRect(x, y, w, h); }
    void width(int w) { _rect.w = w; }
    void height(int h) { _rect.h = h; }

    void drawChildren() const;

public:
    Element(const SDL_Rect &rect);
    ~Element();

    static TTF_Font *font() { return _font; }
    static void font(TTF_Font *newFont) { _font = newFont; }

    void addChild(Element *child) { _children.push_back(child); };

    void makeBackgroundTransparent();

    virtual void onMouseDown(const Point &mousePos); // React to a mouse button being pressed down
    virtual void onMouseUp(const Point &mousePos); // React to a mouse button being pressed down
    virtual void onMouseMove(const Point &mousePos); // React to a mouse button being pressed down

    virtual void draw(); // Draw the existing texture to its designated location.

    static void cleanup(); // Release static resources
};

#endif
