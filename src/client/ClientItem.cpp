// (C) 2015-2016 Tim Gurto 

#include "ClientItem.h"

ClientItem::ClientItem(const std::string &id, const std::string &name):
_id(id),
_name(name),
_constructsObject(nullptr){}

void ClientItem::icon(const std::string &filename){
    static const std::string
        prefix = "Images/Items/",
        suffix = ".png";
    _icon = Texture(prefix + filename + suffix, Color::MAGENTA);
}

void ClientItem::addClass(const std::string &className){
    _classes.insert(className);
}

bool ClientItem::isClass(const std::string &className) const{
    return _classes.find(className) != _classes.end();
}
