#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <map>

#include "../Serial.h"

class Entity;

// A subdivision of the map, used
class CollisionChunk {
  std::map<Serial, const Entity *>
      _entities;  // Sorted by serial, for fast removal

 public:
  void addEntity(const Entity *obj);
  void removeEntity(Serial serial);
  const std::map<Serial, const Entity *> entities() const { return _entities; }
};

typedef std::map<size_t, std::map<size_t, CollisionChunk> > CollisionGrid;

#endif
