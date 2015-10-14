// (C) 2015 Tim Gurto 

#include "Item.h"

Item::Item(const std::string &id, const std::string &name):
_id(id),
_name(name){}

void Item::icon(const std::string &filename){
    static const std::string
        prefix = "Images/Items/",
        suffix = ".png";
    _icon = Texture(prefix + filename + suffix, Color::MAGENTA);
}

void Item::addClass(const std::string &className){
    _classes.insert(className);
}

void Item::addMaterial(const Item *id, size_t quantity){
    _materials[id] = quantity;
}

