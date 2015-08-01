// (C) 2015 Tim Gurto

#ifndef TREE_LITE_H
#define TREE_LITE_H

#include "Point.h"

// A server-side representation of a branch
struct TreeLite{
    size_t serial;
    Point location;

    static const Uint32 ACTION_TIME;

    TreeLite(const TreeLite &rhs);

    TreeLite(const Point &loc, size_t wood); // Generates new serial

    TreeLite(size_t s); // For set/map lookup

    bool operator<(const TreeLite &rhs) const { return serial < rhs.serial; }

    size_t wood() const { return _wood; }

    void decrementWood();

private:
    static size_t _currentSerial;

    size_t _wood; // Amount of wood remaining
};

#endif
