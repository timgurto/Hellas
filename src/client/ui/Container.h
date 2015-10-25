// (C) 2015 Tim Gurto

#ifndef CONTAINER_H
#define CONTAINER_H

#include "Element.h"
#include "../Client.h"
#include "../../server/Item.h"

// A grid that allows access to a collection of items
class Container : public Element{
    static const int GAP;

    size_t _rows, _cols;
    Item::vect_t &_linked;

    // For these, INVENTORY_SIZE = none
    size_t
        _leftMouseDownSlot, // The slot that the left mouse button went down on, if any.
        _leftClickSlot; // The slot that was left-clicked, if any.

    virtual void refresh() override;

    static void mouseDown(Element &e, const Point &mousePos);
    static void mouseUp(Element &e, const Point &mousePos);

    size_t getSlot(const Point &mousePos) const;

public:
    Container(size_t rows, size_t cols, Item::vect_t &linked, int x = 0, int y = 0);

    size_t leftClickSlot() const { return _leftClickSlot; }
    void clearMouseDown();
};

#endif
