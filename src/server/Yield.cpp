// (C) 2015 Tim Gurto

#include "Yield.h"
#include "../util.h"

std::default_random_engine Yield::generator;

void Yield::addItem(const ServerItem *item, double initMean, double initSD, double gatherMean,
                    double gatherSD){
    YieldEntry *entry = &_entries[item];
    entry->_initMean = initMean;
    entry->_initSD = initSD;
    entry->_gatherMean = gatherMean;
    entry->_gatherSD = gatherSD;
    entry->_gatherDistribution = std::normal_distribution<double>(gatherMean, gatherSD);
}

void Yield::instantiate(ItemSet &contents) const{
    for (auto entry : _entries) {
        contents.set(entry.first, generateInitialQuantity(entry.second));
    }
}

size_t Yield::generateInitialQuantity(const YieldEntry &entry){
    double d = std::normal_distribution<double>(entry._initMean, entry._initSD)(generator);
    return toInt(max<double>(0, d));
}

size_t Yield::generateGatherQuantity(const ServerItem *item) const{
    const YieldEntry &entry = _entries.find(item)->second;
    double d = entry._gatherDistribution(generator);
    return max<size_t>(1, toInt(d)); // User always gets at least one item when gathering
}
