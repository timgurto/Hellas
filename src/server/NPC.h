// (C) 2016 Tim Gurto

#ifndef NPC_H
#define NPC_H

#include "../Point.h"

class NPC{
    size_t _serial; // Starts at 1; 0 is reserved.
    Point _location;

protected:
    static size_t generateSerial();

public:
    NPC(size_t serial); // For set/map lookup; contains only a serial
    NPC(const Point &loc); // For set/map lookup; contains only a location

    static const Rect COLLISION_RECT;

    const Point &location() const { return _location; }
    size_t serial() const { return _serial; }
    void serial(size_t s) { _serial = s; }

    bool operator<(const NPC &rhs) const { return _serial < rhs._serial; }
};

#endif
