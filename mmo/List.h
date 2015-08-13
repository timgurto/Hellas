// (C) 2015 Tim Gurto

#ifndef LIST_H
#define LIST_H

#include "Element.h"

// A scrollable vertical list of elements of uniform height
class List : public Element{
    static const int SCROLL_BAR_WIDTH = 10;

    int _childHeight;

    Element *_content; // Holds the list items themselves, and moves up and down to "scroll".

    virtual void refresh();

public:
    List(const SDL_Rect &rect, int childHeight);

    virtual void addChild(Element *child);
    virtual Element *findChild(const std::string id);
};

#endif
