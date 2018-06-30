#ifndef LOOT_TABLE_H
#define LOOT_TABLE_H

#include <map>

#include "../NormalVariable.h"
#include "ServerItem.h"

class Loot;

// Defines the loot chances for a single NPC type, and generates loot.
class LootTable {
  struct LootEntry {
    const ServerItem *item;
    NormalVariable normalDist;
    double simpleChance = 0;  // Use this instead, if != 0
  };

  std::vector<LootEntry> _entries;

 public:
  void addNormalItem(const ServerItem *item, double mean, double sd = 0);
  void addSimpleItem(const ServerItem *item, double chance);

  // Creates a new instance of this Yield, with random init values, in the
  // specified ItemSet
  void instantiate(Loot &container) const;
};

#endif
