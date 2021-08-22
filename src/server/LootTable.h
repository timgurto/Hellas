#ifndef LOOT_TABLE_H
#define LOOT_TABLE_H

#include <map>
#include <memory>

#include "../NormalVariable.h"
#include "ServerItem.h"

class Loot;
class User;

// Defines the loot chances for a single NPC type, and generates loot.
class LootTable {
  struct ILootEntry {
    virtual bool operator==(const ILootEntry &rhs) const = 0;
    bool operator!=(const ILootEntry &rhs) const { return !((*this) == rhs); }
    virtual std::pair<const ServerItem *, int> instantiate() const = 0;
  };

  struct SimpleEntry : public ILootEntry {  // x% chance to receive y item
    const ServerItem *item{nullptr};
    double chance{0};

    bool operator==(const ILootEntry &rhs) const override;
    std::pair<const ServerItem *, int> instantiate() const override;
  };

  struct NormalEntry : public ILootEntry {  // normal distribution
    const ServerItem *item;
    NormalVariable normalDist;

    bool operator==(const ILootEntry &rhs) const override;
    std::pair<const ServerItem *, int> instantiate() const override;
  };

  struct ChoiceEntry : public ILootEntry {  // receive one of these items
    std::vector<std::pair<const ServerItem *, int>> choices;

    bool operator==(const ILootEntry &rhs) const override;
    std::pair<const ServerItem *, int> instantiate() const override;
  };

  std::vector<std::shared_ptr<ILootEntry>> _entries;

  const LootTable *_nestedTable{nullptr};

 public:
  bool operator==(const LootTable &rhs) const;
  bool operator!=(const LootTable &rhs) const { return !((*this) == rhs); }

  void addNormalItem(const ServerItem *item, double mean, double sd = 0);
  void addSimpleItem(const ServerItem *item, double chance);
  void addChoiceOfItems(
      const std::vector<std::pair<const ServerItem *, int>> choices);
  void addNestedLootTable(const LootTable &table) { _nestedTable = &table; }

  // Creates a new instance of this Yield, with random init values, in the
  // specified ItemSet
  void instantiate(Loot &container, const User *killer) const;
};

#endif
