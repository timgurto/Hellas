// (C) 2015 Tim Gurto

#ifndef CHEST_LITE_H
#define CHEST_LITE_H

#include "Point.h"

// A server-side representation of a chest
struct ChestLite{
    size_t serial;
    Point location;

    ChestLite(const ChestLite &rhs);

    ChestLite(const Point &loc); // Generates new serial

    ChestLite(size_t s); // For set/map lookup

    bool operator<(const ChestLite &rhs) const { return serial < rhs.serial; }

private:
    static size_t _currentSerial;
};

#endif
