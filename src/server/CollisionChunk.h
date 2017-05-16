// (C) 2015 Tim Gurto

#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <map>

class Entity;

// A subdivision of the map, used
class CollisionChunk{
    std::map<size_t, const Entity *> _entities; // Sorted by serial, for fast removal

public:
    void addEntity(const Entity *obj);
    void removeEntity(size_t serial);
    const std::map<size_t, const Entity *> entities() const { return _entities; }
};

typedef std::map<size_t, std::map<size_t, CollisionChunk> > CollisionGrid;

#endif
