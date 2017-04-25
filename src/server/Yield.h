#ifndef YIELD_H
#define YIELD_H

#include <map>

#include "ItemSet.h"
#include "ServerItem.h"
#include "../NormalVariable.h"

// Provides a way for objects to give players items when gathered.  This is part of an ObjectType.
class Yield{
    struct YieldEntry{
        NormalVariable _initDistribution, _gatherDistribution;
        size_t _initMin;
        double _gatherMean;
    };

    std::map<const ServerItem *, YieldEntry> _entries;

public:
    operator bool() const {return !_entries.empty(); }

    double gatherMean(const ServerItem *item) const;
    
    void addItem(const ServerItem *item,
                 double initMean = 1, double initSD = 0, size_t initMin = 0,
                 double gatherMean = 1., double gatherSD = 0);

    // Creates a new instance of this Yield, with random init values, in the specified ItemSet
    void instantiate(ItemSet &contents) const; 

    // Generate a normally-distributed random number based on the mean and SD of an entry
    static size_t generateInitialQuantity(const YieldEntry &entry);
    size_t generateGatherQuantity(const ServerItem *item) const;
};

#endif
