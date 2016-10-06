// (C) 2015 Tim Gurto

#include <cassert>

#include "Yield.h"
#include "../util.h"

void Yield::addItem(const ServerItem *item, double initMean, double initSD, double gatherMean,
                    double gatherSD){
    /*this;
    _entries;
    _entries.size();
    auto map = _entries;
    map[item];*/
    //_entries.insert(std::make_pair(item, YieldEntry()));
    _entries[item];
    YieldEntry *entry = &_entries[item];
    entry->_initDistribution = NormalVariable(initMean, initSD);
    entry->_gatherDistribution = NormalVariable(gatherMean, gatherSD);
    entry->_gatherMean = gatherMean;
}

void Yield::instantiate(ItemSet &contents) const{
    for (auto entry : _entries) {
        contents.set(entry.first, generateInitialQuantity(entry.second));
    }
}

size_t Yield::generateInitialQuantity(const YieldEntry &entry){
    double d = entry._initDistribution();
    return toInt(max<double>(0, d));
}

size_t Yield::generateGatherQuantity(const ServerItem *item) const{
    const YieldEntry &entry = _entries.find(item)->second;
    double d = entry._gatherDistribution();
    return max<size_t>(1, toInt(d)); // User always gets at least one item when gathering
}

double Yield::gatherMean(const ServerItem *item) const{
    auto it = _entries.find(item);
    assert (it != _entries.end());
    return it->second._gatherMean;
}
