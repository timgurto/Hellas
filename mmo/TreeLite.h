#ifndef TREE_LITE_H
#define TREE_LITE_H

#include "Point.h"

// A server-side representation of a branch
struct TreeLite{
    size_t serial;
    Point location;

    TreeLite(const TreeLite &rhs);

    TreeLite(const Point &loc); // Generates new serial

    TreeLite(size_t s); // For set/map lookup

    inline bool operator<(const TreeLite &rhs) const { return serial < rhs.serial; }

private:
    static size_t _currentSerial;
};

#endif
