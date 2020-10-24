#include "LootTable.h"

#include "../util.h"
#include "Loot.h"
#include "Server.h"
#include "User.h"

bool LootTable::operator==(const LootTable &rhs) const {
  if (_entries.size() != rhs._entries.size()) return false;
  auto other = rhs._entries;  // copy to modify
  for (const auto &entry : _entries) {
    auto entryFound = false;
    for (auto i = 0; i != other.size(); ++i) {
      if (other[i] != entry) continue;

      // Remove from vector
      other[i] = other.back();
      other.pop_back();
      entryFound = true;
      break;
    }

    if (!entryFound) return false;
  }
  return true;
}

void LootTable::addNormalItem(const ServerItem *item, double mean, double sd) {
  _entries.push_back({});
  LootEntry &le = _entries.back();
  le.item = item;
  le.normalDist = {mean, sd};
}

void LootTable::addSimpleItem(const ServerItem *item, double chance) {
  _entries.push_back({});
  LootEntry &le = _entries.back();
  le.item = item;
  le.simpleChance = chance;
}

void LootTable::addChoice(const std::vector<const ServerItem *> choices) {
  LootEntry entry;
  entry.choices = choices;
  _entries.push_back(entry);
}

void LootTable::addAllFrom(const LootTable &rhs) {
  for (const auto &entry : rhs._entries) _entries.push_back(entry);
}

void LootTable::instantiate(Loot &loot, const User *killer) const {
  if (!loot.empty()) {
    SERVER_ERROR("Loot object provided was not empty");
  }
  for (const LootEntry &entry : _entries) {
    const auto NO_LIMIT = -1;
    auto qtyLimit = NO_LIMIT;

    if (killer) {
      // Enforce quest exclusivity
      if (entry.item->isQuestExclusive()) {
        if (!killer->isOnQuest(entry.item->exclusiveToQuest())) return;
      }

      // Limit quantity based on killer's quest progress
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
    }

    if (!entry.choices.empty()) {
      const auto numChoices = entry.choices.size();
      const auto randomIndex = rand() % numChoices;
      const auto chosenItem = entry.choices[randomIndex];
      loot.add(chosenItem, 1);
      continue;
    }

    auto quantity = 0;
    if (entry.simpleChance == 0) {
      double rawQuantity = entry.normalDist.generate();
      quantity = toInt(max<double>(0, rawQuantity));
    } else {
      if (randDouble() < entry.simpleChance) quantity = 1;
    }

    if (qtyLimit != NO_LIMIT) quantity = min(quantity, qtyLimit);

    if (quantity == 0) continue;
    loot.add(entry.item, quantity);
  }
}

bool LootTable::LootEntry::operator==(const LootEntry &rhs) const {
  return item == rhs.item && simpleChance == rhs.simpleChance &&
         normalDist == rhs.normalDist;
}
