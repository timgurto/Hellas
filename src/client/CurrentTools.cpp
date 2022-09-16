#include "CurrentTools.h"
#include "Client.h"

void CurrentTools::update(ms_t timeElapsed) {
  if (_timeSinceLastUpdate >= UPDATE_TIME) {
    _timeSinceLastUpdate = 0;
    lookForTools();
  } else {
    _timeSinceLastUpdate += timeElapsed;
  }
}

void CurrentTools::lookForTools() {
  _toolsMutex.lock();

  const auto previousTools = _tools;

  _tools.clear();

  includeItems();
  includeObjects();
  includeTerrain();

  if (_tools != previousTools) _hasChanged = true;

  _toolsMutex.unlock();

  // This calls hasTool(), which uses the mutex, hence its being unlocked first.
  if (_hasChanged && _client._detailsPane) _client.refreshRecipeDetailsPane();
}

void CurrentTools::includeItems() {
  auto includeItemsInSpecificVect = [&](const ClientItem::vect_t &vect) {
    for (const auto &slot : vect) {
      const auto *type = slot.first.type();
      if (!type) continue;
      if (slot.first.health() == 0) continue;

      includeAllTagsFrom(*type);
    }
  };

  includeItemsInSpecificVect(_client._inventory);
  includeItemsInSpecificVect(_client._character.gear());
}

void CurrentTools::includeObjects() {
  for (const auto *entity : _client._entities) {
    const auto *obj = dynamic_cast<const ClientObject *>(entity);
    if (!obj) continue;
    if (obj->isDead()) continue;
    if (!obj->userHasAccess()) continue;
    if (obj->isBeingConstructed()) continue;
    if (distance(*obj, _client._character) > Client::ACTION_DISTANCE) continue;
    includeAllTagsFrom(*obj->objectType());
  }
}

void CurrentTools::includeTerrain() {
  const auto nearbyTerrain = _client._map.terrainTypesOverlapping(
      _client._character.collisionRect(), Client::ACTION_DISTANCE);
  for (char terrainType : nearbyTerrain) {
    const auto it = _client.gameData.terrain.find(terrainType);
    if (it != _client.gameData.terrain.end()) includeAllTagsFrom(it->second);
  }
}

void CurrentTools::includeAllTagsFrom(const HasTags &thingWithTags) {
  for (const auto &tagPair : thingWithTags.tags()) _tools.insert(tagPair.first);
}

bool CurrentTools::hasTool(std::string tag) const {
  _toolsMutex.lock();
  const auto wasFound = _tools.count(tag) > 0;
  _toolsMutex.unlock();
  return wasFound;
}

bool CurrentTools::hasAnyTools() const {
  _toolsMutex.lock();
  const auto isEmpty = _tools.empty();
  _toolsMutex.unlock();
  return !isEmpty;
}

bool CurrentTools::hasChanged() const {
  const auto ret = _hasChanged;
  _hasChanged = false;
  return ret;
}
