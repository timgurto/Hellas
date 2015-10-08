// (C) 2015 Tim Gurto

#include "Item.h"
#include "Client.h"
#include "Color.h"
#include "ObjectType.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
_id(idArg),
_name(nameArg),
_stackSize(stackSizeArg),
_craftTime(0),
_constructsObject(0){}

Item::Item(const std::string &idArg):
_id(idArg){}

void Item::icon(const std::string &filename){
    static const std::string
        prefix = "Images/Items/",
        suffix = ".png";
    _icon = Texture(prefix + filename + suffix, Color::MAGENTA);
}

void Item::addClass(const std::string &className){
    _classes.insert(className);
}

bool Item::isClass(const std::string &className) const{
    return _classes.find(className) != _classes.end();
}

void Item::addMaterial(const Item *id, size_t quantity){
    _materials[id] = quantity;
}
