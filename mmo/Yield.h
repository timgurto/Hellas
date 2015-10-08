// (C) 2015 Tim Gurto

#ifndef YIELD_H
#define YIELD_H

#include <map>
#include <random>

#include "Item.h"

// Provides a way for objects to give players items when gathered.  This is part of an ObjectType.
class Yield{
    struct YieldEntry{
        double _initMean, _initSD; // The initial distribution of an Item in a Yield
        double _gatherMean, _gatherSD; // The distribution of individual gathers of an Item
    };

    std::map<const Item *, YieldEntry> _entries;

    static std::default_random_engine generator;

public:
    typedef std::map<const Item *, size_t> contents_t;

    operator bool() const {return !_entries.empty(); }
    
    void addItem(const Item *item, double initMean = 1, double initSD = 0,
                 double gatherMean = 1., double gatherSD = 0);

    // Returns a new instance of this Yield, with random init values
    void instantiate(contents_t &contents) const; 

    // Generate a normally-distributed random number based on the initMean and initSD of an entry
    static size_t generateQuantity(const YieldEntry &entry);
};

#endif
