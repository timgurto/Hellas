// (C) 2015 Tim Gurto

#ifndef LIST_H
#define LIST_H

#include "Element.h"
#include "Picture.h"

// A scrollable vertical list of elements of uniform height
class List : public Element{
public:
    static const int
        ARROW_W,
        ARROW_H,
        CURSOR_HEIGHT,
        SCROLL_AMOUNT;

protected:
    static void mouseUp(Element &e, const Point &mousePos);

private:
    bool _mouseDownOnCursor;
    int _cursorOffset; // y-offset of the mouse on the cursor.

    int _childHeight;

    Element *_scrollBar;
    Picture // Arrow images
        *_whiteUp,
        *_whiteDown,
        *_greyUp,
        *_greyDown;
    Element *_cursor; // Shows position on the scroll bar, and can be dragged.
    void updateScrollBar(); // Update the appearance of the scroll bar.
    bool _scrolledToBottom;

    static void cursorMouseDown(Element &e, const Point &mousePos);
    static void mouseMove(Element &e, const Point &mousePos);
    static void scrollUpRaw(Element &e);
    static void scrollDownRaw(Element &e);
    static void scrollUp(Element &e, const Point &mousePos) { scrollUpRaw(e); }
    static void scrollDown(Element &e, const Point &mousePos) { scrollDownRaw(e); }

protected:
    int childHeight() const { return _childHeight; }

    Element *_content; // Holds the list items themselves, and moves up and down to "scroll".

public:
    List(const Rect &rect, int childHeight);

    virtual void addChild(Element *child);
    virtual void clearChildren();
    virtual Element *findChild(const std::string id);

    virtual void refresh();
};

#endif
