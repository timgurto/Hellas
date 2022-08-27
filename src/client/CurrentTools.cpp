#include "CurrentTools.h"
#include "Client.h"

void CurrentTools::update(ms_t timeElapsed) {
  if (_timeUntilNextUpdate > UPDATE_TIME) {
    _timeUntilNextUpdate -= UPDATE_TIME;
    return;
  }
  _timeUntilNextUpdate = UPDATE_TIME;

  _toolsMutex.lock();
  _tools.clear();

  const auto *type = _client._inventory[0].first.type();
  const auto firstInventorySlotHasItem = type != nullptr;
  if (firstInventorySlotHasItem) {
    const auto speed = 0.0;
    for (const auto &tagPair : type->tags()) _tools.insert(tagPair.first);
  }

  _toolsMutex.unlock();
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
