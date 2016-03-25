// (C) 2015-2016 Tim Gurto

#include "Recipe.h"

Recipe::Recipe(const std::string &id):
_id(id),
_product(nullptr),
_time(0){}

void Recipe::addMaterial(const Item *item, size_t qty){
    _materials.add(item, qty);
}

void Recipe::addTool(const std::string &name){
    _tools.insert(name);
}
