// (C) 2015 Tim Gurto

#ifndef OBJECT_H
#define OBJECT_H

#include "ObjectType.h"
#include "Point.h"

// A server-side representation of an in-game object
class Object{
    size_t _serial;
    Point _location;
    const ObjectType &_type;

    Uint32
        _gatherTime;

protected:
    static size_t generateSerial();

public:
    Object(const Object &rhs);

    Object(const ObjectType &type, const Point &loc, Uint32 gatherTime = 0); // Generates new serial

    static ObjectType emptyType; // Used for below dummy-lookup items
    Object(size_t serial); // For set/map lookup; contains only a serial

    bool operator<(const Object &rhs) const { return _serial < rhs._serial; }
};

#endif
