#ifndef LOOT_TABLE_H
#define LOOT_TABLE_H

#include <map>

#include "ServerItem.h"
#include "../NormalVariable.h"

class Loot;

// Defines the loot chances for a single NPC type, and generates loot.
class LootTable{
    struct LootEntry{
        const ServerItem *item;
        NormalVariable normalDist;
    };

    std::vector<LootEntry> _entries;

public:    
    void addItem(const ServerItem *item, double mean = 1, double sd = 0);

    // Creates a new instance of this Yield, with random init values, in the specified ItemSet
    void instantiate(Loot &container) const;
};

#endif
