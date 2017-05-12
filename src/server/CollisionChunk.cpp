#include <cassert>

#include "CollisionChunk.h"
#include "objects/Object.h"

void CollisionChunk::addObject(const Object *obj){
    _objects[obj->serial()] = obj;
}

void CollisionChunk::removeObject(size_t serial){
    auto it = _objects.find(serial);
    if (it != _objects.end())
        _objects.erase(it);
}
