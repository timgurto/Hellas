// (C) 2015 Tim Gurto

#include "Object.h"
#include "util.h"

ObjectType Object::emptyType("");

Object::Object(const ObjectType &type, const Point &loc):
_serial(generateSerial()),
_location(loc),
_type(type),
_wood(type.wood()){}

Object::Object(const ObjectType &type, const Point &loc, size_t wood):
_serial(generateSerial()),
_location(loc),
_type(type),
_wood(wood){}

Object::Object(size_t serial): // For set/map lookup
_serial (serial),
_type(emptyType){}

size_t Object::generateSerial() {
    static size_t currentSerial = 0;
    return currentSerial++;
}

void Object::decrementWood(){
    --_wood;
}
