// (C) 2015 Tim Gurto

#include <cassert>

#include "Object.h"
#include "util.h"

Object::Object(const ObjectType *type, const Point &loc):
_serial(generateSerial()),
_location(loc),
_type(type){
    assert(type);
    if (type->yield())
        type->yield().instantiate(_contents);
}

Object::Object(size_t serial): // For set/map lookup
_serial (serial),
_type(0){}

size_t Object::generateSerial() {
    static size_t currentSerial = 0;
    return currentSerial++;
}

void Object::removeItem(const Item *item){
    auto it = _contents.find(item);
    --it->second;
    if (it->second == 0)
        _contents.erase(it);
}

const Item *Object::chooseGatherItem() const{
    assert(!_contents.empty());
    // TODO: choose randomly
    return _contents.begin()->first;
}
