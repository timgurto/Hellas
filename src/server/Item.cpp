// (C) 2015 Tim Gurto

#include "Item.h"
#include "ObjectType.h"

Item::Item(const std::string &idArg):
_id(idArg){}

void Item::addClass(const std::string &className){
    _classes.insert(className);
}

bool Item::isClass(const std::string &className) const{
    return _classes.find(className) != _classes.end();
}

void Item::addMaterial(const Item *id, size_t quantity){
    _materials[id] = quantity;
}

