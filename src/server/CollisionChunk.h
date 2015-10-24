// (C) 2015 Tim Gurto

#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <map>

// A subdivision of the map, used
class CollisionChunk{
    mutable std::map<size_t, std::map<size_t, bool> > _passableTiles; // x,y -> passable?

public:
    void addTile(size_t x, size_t y, bool isPassable);
    bool isTilePassable(size_t x, size_t y) const;
};

typedef std::map<size_t, std::map<size_t, CollisionChunk> > CollisionGrid;

#endif
