// (C) 2015 Tim Gurto

#include "Yield.h"

std::default_random_engine Yield::generator;

void Yield::addItem(const Item *item, double initMean, double initSD, double gatherMean,
                    double gatherSD){
    YieldEntry *entry = &_entries[item];
    entry->_initMean = initMean;
    entry->_initSD = initSD;
    entry->_gatherMean = gatherMean;
    entry->_gatherSD = gatherSD;
}

void Yield::instantiate(contents_t &contents) const{
    for (auto entry : _entries) {
        contents[entry.first] = generateQuantity(entry.second);
    }
}

size_t Yield::generateQuantity(const YieldEntry &entry){
    double d = std::normal_distribution<double>(entry._initMean, entry._initSD)(generator);
    return max<size_t>(0, toInt(d));
}
