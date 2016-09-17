// (C) 2015-2016 Tim Gurto

#ifndef CLIENT_RECIPE_H
#define CLIENT_RECIPE_H

#include <SDL.h>
#include <set>
#include <string>

#include "ClientItemSet.h"
#include "../types.h"

// Couples materials with products that players can craft.
class ClientRecipe{
    std::string _id;
    ClientItemSet _materials;
    std::set<std::string> _tools; // Tools required for crafting
    const ClientItem *_product;
    ms_t _time;

public:
    ClientRecipe(const std::string &id); //time = 0, ptrs = nullptr

    bool operator<(const ClientRecipe &rhs) const { return _id < rhs._id; }

    const std::string &id() const { return _id; }
    const ClientItemSet &materials() const { return _materials; }
    const std::set<std::string> & tools() const { return _tools; }
    const ClientItem *product() const { return _product; }
    void product(const ClientItem *item) { _product = item; }
    ms_t time() const { return _time; }
    void time(ms_t time) { _time = time; }

    void addMaterial(const ClientItem *item, size_t qty);
    void addTool(const std::string &name);
};

#endif
