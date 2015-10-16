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

    virtual void refresh();

public:
    Container(size_t rows, size_t cols, Item::vect_t &linked, int x = 0, int y = 0);
};

#endif
