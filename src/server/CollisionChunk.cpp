// (C) 2015 Tim Gurto

#include <cassert>

#include "CollisionChunk.h"
#include "Object.h"

void CollisionChunk::addTile(size_t x, size_t y, bool isPassable){
    _passableTiles[x][y] = isPassable;
}

bool CollisionChunk::isTilePassable(size_t x, size_t y) const{
    assert(_passableTiles.find(x) != _passableTiles.end());
    assert(_passableTiles[x].find(y) != _passableTiles[x].end());
    return _passableTiles[x][y];
}

void CollisionChunk::addObject(const Object *obj){
    _objects[obj->serial()] = obj;
}
