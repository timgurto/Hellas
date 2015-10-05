// (C) 2015 Tim Gurto

#ifndef CONTAINER_H
#define CONTAINER_H

#include <vector>

#include "Client.h"
#include "Element.h"

class Item;

// A grid that allows access to a collection of items
class Container : public Element{
public:
    typedef std::vector<std::pair<const Item *, size_t> > containerVec_t;

private:
    static const int GAP;

    size_t _rows, _cols;
    containerVec_t &_linked;

public:
    Container(size_t rows, size_t cols, containerVec_t &linked, int x = 0, int y = 0);
};

#endif
