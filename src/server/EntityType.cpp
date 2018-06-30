#include "EntityType.h"

EntityType::EntityType(const std::string id)
    : _id(id), _collides(false), _allowedTerrain(nullptr) {}

void EntityType::addTag(const std::string &tagName) { _tags.insert(tagName); }

bool EntityType::isTag(const std::string &tagName) const {
  return _tags.find(tagName) != _tags.end();
}

const TerrainList &EntityType::allowedTerrain() const {
  if (_allowedTerrain == nullptr)
    return TerrainList::defaultList();
  else
    return *_allowedTerrain;
}
