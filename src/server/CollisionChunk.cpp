// (C) 2015 Tim Gurto

#include <cassert>

#include "CollisionChunk.h"
#include "Object.h"

void CollisionChunk::addObject(const Object *obj){
    _objects[obj->serial()] = obj;
}

void CollisionChunk::removeObject(size_t serial){
    auto it = _objects.find(serial);
    if (it != _objects.end())
        _objects.erase(it);
}
