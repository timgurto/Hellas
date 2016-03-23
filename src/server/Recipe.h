// (C) 2015 Tim Gurto

#ifndef RECIPE_H
#define RECIPE_H

#include <SDL.h>
#include <set>
#include <string>

#include "ItemSet.h"
#include "../types.h"

// Couples materials with products that players can craft.
class Recipe{
    std::string _id;
    ItemSet _materials;
    std::set<std::string> _tools; // Tools required for crafting
    const Item *_product;
    ms_t _time;

public:
    Recipe(const std::string &id); //time=0, ptrs = 0

    bool operator<(const Recipe &rhs) const { return _id < rhs._id; }

    const std::string &id() const { return _id; }
    const ItemSet &materials() const { return _materials; }
    const std::set<std::string> & tools() const { return _tools; }
    const Item *product() const { return _product; }
    void product(const Item *item) { _product = item; }
    ms_t time() const { return _time; }
    void time(ms_t time) { _time = time; }

    void addMaterial(const Item *item, size_t qty);
    void addTool(const std::string &name);
};

#endif
