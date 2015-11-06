// (C) 2015 Tim Gurto 

#include "Item.h"

Item::Item(const std::string &id, const std::string &name):
_id(id),
_name(name),
_constructsObject(0){}

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
