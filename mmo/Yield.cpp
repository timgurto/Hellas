// (C) 2015 Tim Gurto

#include "Yield.h"

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

size_t Yield::generateQuantity(const YieldEntry entry){
    return entry._initMean; // TODO: generate random quantities.
}
