#include <cassert>

#include "Loot.h"
#include "LootTable.h"
#include "../util.h"

void LootTable::addItem(const ServerItem *item, double mean, double sd){
    _entries.push_back(LootEntry());
    LootEntry &le = _entries.back();
    le.item = item;
    le.normalDist = NormalVariable(mean, sd);
}

void LootTable::instantiate(Loot &loot) const{
    assert(loot.empty());
    size_t i = 0;
    for (const LootEntry &entry : _entries){
        double rawQuantity = entry.normalDist.generate();
        size_t actualQuantity = toInt(max<double>(0, rawQuantity));
        loot.add(entry.item, actualQuantity);
    }
}
