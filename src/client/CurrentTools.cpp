#include "CurrentTools.h"
#include "Client.h"

void CurrentTools::update(ms_t timeElapsed) {
  if (_timeUntilNextUpdate > UPDATE_TIME) {
    _timeUntilNextUpdate -= UPDATE_TIME;
    return;
  }
  _timeUntilNextUpdate = UPDATE_TIME;

  lookForTools();
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
    includeTags(*obj->objectType());
  }
}

void CurrentTools::includeTerrain() {}

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
