#include "EntityType.h"

EntityType::EntityType(const std::string id)
    : _id(id), _collides(false), _allowedTerrain(nullptr) {}

const TerrainList &EntityType::allowedTerrain() const {
  if (_allowedTerrain == nullptr)
    return TerrainList::defaultList();
  else
    return *_allowedTerrain;
}
