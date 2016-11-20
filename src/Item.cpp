#include "Item.h"

Item::Item(const std::string &id):
_id(id){}

void Item::addTag(const std::string &tagName){
    _tags.insert(tagName);
}

bool Item::isTag(const std::string &tagName) const{
    return _tags.find(tagName) != _tags.end();
}
