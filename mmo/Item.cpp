// (C) 2015 Tim Gurto

#include "Item.h"
#include "Client.h"
#include "Color.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
_id(idArg),
_name(nameArg),
_stackSize(stackSizeArg),
_icon(std::string("Images/") + _id + ".png", Color::MAGENTA),
_craftTime(0){}

Item::Item(const std::string &idArg):
_id(idArg){}

void Item::addClass(const std::string &className){
    _classes.insert(className);
}

bool Item::isClass(const std::string &className) const{
    return _classes.find(className) != _classes.end();
}

void Item::addMaterial(const std::string &id, size_t quantity){
    _materials[id] = quantity;
}
