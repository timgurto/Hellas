#include <cassert>

#include "../util.h"
#include "Loot.h"
#include "LootTable.h"
#include "Server.h"
#include "User.h"

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

void LootTable::instantiate(Loot &loot, const User *killer) const {
  assert(loot.empty());
  for (const LootEntry &entry : _entries) {
    // Enforce quest exclusivity
    if (entry.item->isQuestExclusive()) {
      if (!killer) continue;
      if (!killer->isOnQuest(entry.item->exclusiveToQuest())) return;
    }

    // Limit quantity based on killer's quest progress
    const auto NO_LIMIT = -1;
    auto qtyLimit = NO_LIMIT;
    if (entry.item->isQuestExclusive()) {
      const auto &server = Server::instance();
      auto quest = server.findQuest(entry.item->exclusiveToQuest());
      for (const auto &objective : quest->objectives) {
        if (objective.type != Quest::Objective::FETCH) continue;
        if (objective.id != entry.item->id()) continue;
        auto numNeededForQuest = objective.qty;
        auto numHeld = killer->countItems(entry.item);
        qtyLimit = numNeededForQuest - numHeld;
        break;
      }
    }

    auto quantity = 0;
    if (entry.simpleChance == 0) {
      double rawQuantity = entry.normalDist.generate();
      quantity = toInt(max<double>(0, rawQuantity));
    } else {
      if (randDouble() < entry.simpleChance) quantity = 1;
    }
    quantity = min(quantity, qtyLimit);
    if (quantity == 0) continue;
    loot.add(entry.item, quantity);
  }
}
