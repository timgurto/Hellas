// (C) 2015 Tim Gurto

#ifndef BRANCH_LITE_H
#define BRANCH_LITE_H

#include "Point.h"

// A server-side representation of a branch
struct BranchLite{
    size_t serial;
    Point location;

    static const Uint32 ACTION_TIME;

    BranchLite(const BranchLite &rhs);

    BranchLite(const Point &loc); // Generates new serial

    BranchLite(size_t s); // For set/map lookup

    inline bool operator<(const BranchLite &rhs) const { return serial < rhs.serial; }

private:
    static size_t _currentSerial;
};

#endif
