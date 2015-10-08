// (C) 2015 Tim Gurto

#ifndef OBJECT_H
#define OBJECT_H

#include "ObjectType.h"
#include "Point.h"
#include "Yield.h"

// A server-side representation of an in-game object
class Object{
    size_t _serial;
    Point _location;
    const ObjectType *_type;
    std::string _owner;
    Yield::contents_t _contents; // Remaining contents, which can be gathered
    size_t _totalContents; // Total quantity of contents remaining.

protected:
    static size_t generateSerial();

public:
    // Both constructors generate new serials
    Object(const ObjectType *type, const Point &loc);
    Object(size_t serial); // For set/map lookup; contains only a serial

    const Point &location() const { return _location; }
    size_t serial() const { return _serial; }
    void serial(size_t s) { _serial = s; }
    const ObjectType *type() const { return _type; }
    const std::string &owner() const { return _owner; }
    void owner(const std::string &name) { _owner = name; }
    const Yield::contents_t &contents() const { return _contents; }
    void contents(Yield::contents_t contents) { _contents = contents; }

    bool operator<(const Object &rhs) const { return _serial < rhs._serial; }

    // Randomly choose an item type for the user to gather.
    const Item *chooseGatherItem() const;
    void removeItem(const Item *item);
};

#endif
