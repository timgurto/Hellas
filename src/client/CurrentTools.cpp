#include "CurrentTools.h"
#include "Client.h"

void CurrentTools::update(ms_t timeElapsed) {
  _tools.clear();
  const auto firstInventorySlotHasItem =
      _client._inventory[0].first.type() != nullptr;
  if (firstInventorySlotHasItem) _tools.insert("");
}
