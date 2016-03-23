// (C) 2015 Tim Gurto

#ifndef CONTAINER_H
#define CONTAINER_H

#include "Element.h"
#include "../Client.h"
#include "../../server/Item.h"

// A grid that allows access to a collection of items
class Container : public Element{
    static const px_t GAP;

    size_t _rows, _cols;
    Item::vect_t &_linked;
    size_t _serial; // The serial of the object with this container.  0 = user's inventory.

    size_t
        _mouseOverSlot, // The slot that the mouse is currently over
        _leftMouseDownSlot, // The slot that the left mouse button went down on, if any.
        _rightMouseDownSlot; // The slot that the left mouse button went down on, if any.

    static const size_t NO_SLOT;

    static Texture _highlight; // Emphasizes any slot the mouse is over.

    static size_t dragSlot; // The slot currently being dragged from.
    static const Container *dragContainer; // The container currently being dragged from.
    
    static size_t useSlot; // The slot whose item is currently being "used" (after right-clicking)
    static const Container *useContainer;

    virtual void refresh() override;
    
    static void leftMouseDown(Element &e, const Point &mousePos);
    static void leftMouseUp(Element &e, const Point &mousePos);
    static void rightMouseDown(Element &e, const Point &mousePos);
    static void rightMouseUp(Element &e, const Point &mousePos);
    static void mouseMove(Element &e, const Point &mousePos);

    size_t getSlot(const Point &mousePos) const;

public:
    Container(size_t rows, size_t cols, Item::vect_t &linked, size_t serial = 0,
              px_t x = 0, px_t y = 0);
    
    static const Item *getDragItem();
    static const Item *getUseItem();
    static void dropItem(); // Drop the item currently being dragged.

    static void clearDragItem();
    static void clearUseItem();

    friend Client;
    friend void Element::cleanup();
};

#endif
