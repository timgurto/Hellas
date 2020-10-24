#ifndef LOOT_TABLE_H
#define LOOT_TABLE_H

#include <map>

#include "../NormalVariable.h"
#include "ServerItem.h"

class Loot;
class User;

// Defines the loot chances for a single NPC type, and generates loot.
class LootTable {
  struct LootEntry {
    const ServerItem *item;
    NormalVariable normalDist;
    double simpleChance = 0;  // Use this instead, if != 0

    bool operator==(const LootEntry &rhs) const;
    bool operator!=(const LootEntry &rhs) const { return !((*this) == rhs); }

    std::vector<const ServerItem *> choices;
  };

  std::vector<LootEntry> _entries;

 public:
  bool operator==(const LootTable &rhs) const;
  bool operator!=(const LootTable &rhs) const { return !((*this) == rhs); }

  void addNormalItem(const ServerItem *item, double mean, double sd = 0);
  void addSimpleItem(const ServerItem *item, double chance);

  void addChoice(const std::vector<const ServerItem *> choices);

  void addAllFrom(const LootTable &rhs);

  // Creates a new instance of this Yield, with random init values, in the
  // specified ItemSet
  void instantiate(Loot &container, const User *killer) const;
};

#endif
