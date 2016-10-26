#include <cassert>

#include "LootTable.h"
#include "../util.h"

void LootTable::addItem(const ServerItem *item, double mean, double sd){
    _entries.push_back(LootEntry());
    LootEntry &le = _entries.back();
    le.item = item;
    le.normalDist = NormalVariable(mean, sd);
}

void LootTable::instantiate(ServerItem::vect_t &container) const{
    for (auto &pair : container){
        assert(pair.first == nullptr);
        assert(pair.second == 0);
    }
    size_t i = 0;
    for (const LootEntry &entry : _entries){
        double rawQty = entry.normalDist.generate();
        size_t qty = toInt(max<double>(0, rawQty));
        size_t stackSize = entry.item->stackSize();
        while (qty > 0){
            size_t thisSlot = min(stackSize, qty);
            auto &pair = container[i];
            pair.first = entry.item;
            pair.second = thisSlot;
            qty -= thisSlot;
            ++i;
            if (i == container.size()){
                assert(false);
                return;
            }
        }
    }
}
