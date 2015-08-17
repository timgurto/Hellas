// (C) 2015 Tim Gurto

#ifndef LIST_H
#define LIST_H

#include "Element.h"
#include "Picture.h"

// A scrollable vertical list of elements of uniform height
class List : public Element{
    static const int
        ARROW_W,
        ARROW_H,
        CURSOR_HEIGHT;

    int _scrollY; // Offset, in pixels; should always be 0 or negative.

    int _childHeight;

    Element *_content; // Holds the list items themselves, and moves up and down to "scroll".
    Element *_scrollBar;
    Picture // Arrow images
        *_whiteUp,
        *_whiteDown,
        *_greyUp,
        *_greyDown;
    Element *_cursor; // Shows position on the scroll bar, and can be dragged.
    void updateCursor(); // Set the cursor location based on _scrollY.

    virtual void refresh();

public:
    List(const SDL_Rect &rect, int childHeight);

    virtual void addChild(Element *child);
    virtual Element *findChild(const std::string id);
};

#endif
