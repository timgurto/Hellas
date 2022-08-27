#include "CurrentTools.h"
#include "Client.h"

void CurrentTools::update(ms_t timeElapsed) {
  if (_timeUntilNextUpdate > UPDATE_TIME) {
    _timeUntilNextUpdate -= UPDATE_TIME;
    return;
  }
  _timeUntilNextUpdate = UPDATE_TIME;

  clearTags();
  const auto firstInventorySlotHasItem =
      _client._inventory[0].first.type() != nullptr;
  if (firstInventorySlotHasItem) addTag("sawing", 0);
}
