// (C) 2015 Tim Gurto

#ifndef ITEM_H
#define ITEM_H

#include <map>
#include <string>
#include <set>
#include <vector>

#include "Texture.h"

// The client-side representation of an item type
class Item{
    std::string _id;
    std::string _name;
    Texture _icon;

    // Used by the crafting window
    std::set<std::string> _classes;
    std::map<const Item *, size_t> _materials;

public:
    Item(const std::string &id, const std::string &name);

    const std::string &name() const { return _name; }
    const Texture &icon() const { return _icon; }

    typedef std::vector<std::pair<const Item *, size_t> > vect_t;
};

#endif