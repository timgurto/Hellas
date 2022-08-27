#include "CurrentTools.h"
#include "Client.h"

void CurrentTools::update(ms_t timeElapsed) {
  if (_timeUntilNextUpdate > UPDATE_TIME) {
    _timeUntilNextUpdate -= UPDATE_TIME;
    return;
  }
  _timeUntilNextUpdate = UPDATE_TIME;

  clearTags();

  const auto *type = _client._inventory[0].first.type();
  const auto firstInventorySlotHasItem = type != nullptr;
  if (!firstInventorySlotHasItem) return;
  const auto arbitraryTag = type->tags().begin()->first;
  const auto speed = 0.0;
  addTag(arbitraryTag, speed);
}
