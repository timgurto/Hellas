#ifndef ITEM_H
#define ITEM_H

#include <set>
#include <string>
#include <vector>

#include "Stats.h"

class Item{
protected:
    std::string _id; // The no-space, unique name used in data files
    std::set<std::string> _classes;
    size_t _gearSlot;
    StatsMod _stats; // If gear, the impact it has on its wearer's stats.

public:
    Item(const std::string &id);
    virtual ~Item(){}

    const std::string &id() const { return _id; }
    const std::set<std::string> &classes() const { return _classes; }
    void gearSlot(size_t slot) { _gearSlot = slot; }
    size_t gearSlot() const { return _gearSlot; }
    void stats(const StatsMod &stats) { _stats = stats; }
    const StatsMod &stats() const { return _stats; }
    
    bool operator<(const Item &rhs) const { return _id < rhs._id; }
    
    typedef std::vector<std::pair<const Item *, size_t> > vect_t;

    void addClass(const std::string &className);
    bool hasClasses() const { return _classes.size() > 0; }
    bool isClass(const std::string &className) const;
};

#endif
