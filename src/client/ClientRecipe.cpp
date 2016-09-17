// (C) 2015-2016 Tim Gurto

#include "ClientRecipe.h"

ClientRecipe::ClientRecipe(const std::string &id):
_id(id),
_product(nullptr),
_time(0){}

void ClientRecipe::addMaterial(const ClientItem *item, size_t qty){
    _materials.add(item, qty);
}

void ClientRecipe::addTool(const std::string &name){
    _tools.insert(name);
}
