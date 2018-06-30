#include <cassert>

#include "../util.h"
#include "Loot.h"
#include "LootTable.h"

void LootTable::addNormalItem(const ServerItem *item, double mean, double sd) {
  _entries.push_back({});
  LootEntry &le = _entries.back();
  le.item = item;
  le.normalDist = NormalVariable(mean, sd);
}

void LootTable::addSimpleItem(const ServerItem *item, double chance) {
  _entries.push_back({});
  LootEntry &le = _entries.back();
  le.item = item;
  le.simpleChance = chance;
}

void LootTable::instantiate(Loot &loot) const {
  assert(loot.empty());
  for (const LootEntry &entry : _entries) {
    auto quantity = 0;
    if (entry.simpleChance == 0) {
      double rawQuantity = entry.normalDist.generate();
      quantity = toInt(max<double>(0, rawQuantity));
    } else {
      if (randDouble() < entry.simpleChance) quantity = 1;
    }
    if (quantity == 0) continue;
    loot.add(entry.item, quantity);
  }
}
