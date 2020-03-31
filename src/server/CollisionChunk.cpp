#include "CollisionChunk.h"

#include "Entity.h"

void CollisionChunk::addEntity(const Entity *obj) {
  _entities[obj->serial()] = obj;
}

void CollisionChunk::removeEntity(Serial serial) {
  auto it = _entities.find(serial);
  if (it != _entities.end()) _entities.erase(it);
}
