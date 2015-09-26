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
    size_t _wood; // wood remaining

protected:
    static size_t generateSerial();

public:
    // Both constructors generate new serials
    Object(const ObjectType &type, const Point &loc);
    Object(const ObjectType &type, const Point &loc, size_t wood); // Load pre-existing from file

    static ObjectType emptyType; // Used for below dummy-lookup items
    Object(size_t serial); // For set/map lookup; contains only a serial

    const Point &location() const { return _location; }
    size_t serial() const { return _serial; }
    void serial(size_t s) { _serial = s; }
    const ObjectType &type() const { return _type; }
    size_t wood() const { return _wood; }

    bool operator<(const Object &rhs) const { return _serial < rhs._serial; }

    void decrementWood();
};

#endif
