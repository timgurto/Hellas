#ifndef ITEM_H
#define ITEM_H

#include <set>
#include <string>
#include <vector>

#include "Stats.h"

class Item{
protected:
    std::string _id; // The no-space, unique name used in data files
    std::set<std::string> _tags;
    size_t _gearSlot;
    StatsMod _stats; // If gear, the impact it has on its wearer's stats.

public:
    Item(const std::string &id);
    virtual ~Item(){}

    const std::string &id() const { return _id; }
    const std::set<std::string> &tags() const { return _tags; }
    void gearSlot(size_t slot) { _gearSlot = slot; }
    size_t gearSlot() const { return _gearSlot; }
    void stats(const StatsMod &stats) { _stats = stats; }
    const StatsMod &stats() const { return _stats; }
    
    bool operator<(const Item &rhs) const { return _id < rhs._id; }
    
    typedef std::vector<std::pair<const Item *, size_t> > vect_t;

    void addTag(const std::string &tagName);
    bool hasTags() const { return _tags.size() > 0; }
    bool isTag(const std::string &tagName) const;
};

#endif
