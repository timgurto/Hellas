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

  _tools.clear();

  includeItems();
  includeObjects();
  includeTerrain();

  _toolsMutex.unlock();
}

void CurrentTools::includeItems() {
  auto includeItemsInSpecificVect = [&](const ClientItem::vect_t &vect) {
    for (const auto &slot : vect) {
      const auto *type = slot.first.type();
      if (!type) continue;

      const auto speed = 0.0;
      includeTags(*type);
    }
  };

  includeItemsInSpecificVect(_client._inventory);
  includeItemsInSpecificVect(_client._character.gear());
}

void CurrentTools::includeObjects() {
  for (const auto *entity : _client._entities) {
    const auto *obj = dynamic_cast<const ClientObject *>(entity);
    if (!obj) continue;
    if (!obj->userHasAccess()) continue;
    if (obj->isBeingConstructed()) continue;
    if (distance(*obj, _client._character) > Client::ACTION_DISTANCE) continue;
    includeTags(*obj->objectType());
  }
}

void CurrentTools::includeTerrain() {
  const auto nearbyTerrain =
      _client._map.terrainTypesOverlapping(_client._character.collisionRect());
  for (char terrainType : nearbyTerrain) {
    const auto it = _client.gameData.terrain.find(terrainType);
    if (it != _client.gameData.terrain.end()) includeTags(it->second);
  }
}

void CurrentTools::includeTags(const HasTags &thingWithTags) {
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