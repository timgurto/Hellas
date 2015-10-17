// (C) 2015 Tim Gurto

#ifndef RECIPE_H
#define RECIPE_H

#include <SDL.h>
#include <string>

#include "ItemSet.h"

// Couples materials with products that players can craft.
class Recipe{
    std::string _id;
    ItemSet _materials;
    const Item *_product;
    Uint32 _time;

public:
    Recipe(const std::string &id); //time=0, ptrs = 0

    bool operator<(const Recipe &rhs) const { return _id < rhs._id; }

    const ItemSet &materials() const { return _materials; }
    const Item *product() const { return _product; }
    void product(const Item *item) { _product = item; }
    Uint32 time() const { return _time; }
    void time(Uint32 time) { _time = time; }

    void addMaterial(const Item *item, size_t qty);
};

#endif
