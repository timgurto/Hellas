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

  auto includeItems = [&](const ClientItem::vect_t &items) {
    for (const auto &slot : items) {
      const auto *type = slot.first.type();
      if (!type) continue;

      const auto speed = 0.0;
      for (const auto &tagPair : type->tags()) _tools.insert(tagPair.first);
    }
  };

  includeItems(_client._inventory);
  includeItems(_client._character.gear());

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
