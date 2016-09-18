// (C) 2015 Tim Gurto

#ifndef YIELD_H
#define YIELD_H

#include <map>
#include <random>

#include "ItemSet.h"
#include "ServerItem.h"

// Provides a way for objects to give players items when gathered.  This is part of an ObjectType.
class Yield{
    struct YieldEntry{
        double _initMean, _initSD; // The initial distribution of an Item in a Yield
        double _gatherMean, _gatherSD; // The distribution of individual gathers of an Item
        mutable std::normal_distribution<double> _gatherDistribution;
    };

    std::map<const ServerItem *, YieldEntry> _entries;

    static std::default_random_engine generator;

public:
    operator bool() const {return !_entries.empty(); }
    
    void addItem(const ServerItem *item, double initMean = 1, double initSD = 0,
                 double gatherMean = 1., double gatherSD = 0);

    // Returns a new instance of this Yield, with random init values
    void instantiate(ItemSet &contents) const; 

    // Generate a normally-distributed random number based on the mean and SD of an entry
    static size_t generateInitialQuantity(const YieldEntry &entry);
    size_t generateGatherQuantity(const ServerItem *item) const;
};

#endif
