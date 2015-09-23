// (C) 2015 Tim Gurto

#include "Object.h"
#include "util.h"

ObjectType Object::emptyType("");

Object::Object(const Object &rhs):
_serial(rhs._serial),
_location(rhs._location),
_type(rhs._type) {}

Object::Object(const ObjectType &type, const Point &loc, Uint32 gatherTime):
_serial(generateSerial()),
_location(loc),
_type(type),
_gatherTime(gatherTime){}

Object::Object(size_t serial): // For set/map lookup
_serial (serial),
_type(emptyType){}

size_t Object::generateSerial() {
    static size_t currentSerial = 0;
    return currentSerial++;
}
