// (C) 2015-2016 Tim Gurto

#include "Item.h"
#include "ObjectType.h"

Item::Item(const std::string &idArg):
_id(idArg),
_stackSize(1),
_constructsObject(nullptr){}

void Item::addClass(const std::string &className){
    _classes.insert(className);
}

bool Item::isClass(const std::string &className) const{
    return _classes.find(className) != _classes.end();
}

bool vectHasSpace(const Item::vect_t &vect, const Item *item, size_t qty){
    for (size_t i = 0; i != vect.size(); ++i) {
        if (vect[i].first == nullptr) {
            if (qty <= item->stackSize())
                return true;
            qty -= item->stackSize();
        } else if (vect[i].first == item) {
            size_t spaceAvailable = item->stackSize() - vect[i].second;
            if (qty <= spaceAvailable)
                return true;
            qty -= spaceAvailable;
        } else
            continue;
    }
    return false;
}
