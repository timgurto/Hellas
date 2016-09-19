// (C) 2016 Tim Gurto

#include "Item.h"

Item::Item(const std::string &id):
_id(id){}

void Item::addClass(const std::string &className){
    _classes.insert(className);
}

bool Item::isClass(const std::string &className) const{
    return _classes.find(className) != _classes.end();
}
