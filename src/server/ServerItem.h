// (C) 2015 Tim Gurto

#ifndef SERVER_ITEM_H
#define SERVER_ITEM_H

#include <SDL.h>
#include <map>

#include "ItemSet.h"
#include "../Item.h"

class ObjectType;

// Describes an item type
class ServerItem : public Item{
    size_t _stackSize;

    // The object that this item can construct
    const ObjectType *_constructsObject;

public:
    ServerItem(const std::string &id);

    typedef std::vector<std::pair<const ServerItem *, size_t> > vect_t;

    size_t stackSize() const { return _stackSize; }
    void stackSize(size_t n) { _stackSize = n; }
    void constructsObject(const ObjectType *obj) { _constructsObject = obj; }
    const ObjectType *constructsObject() const { return _constructsObject; }
    bool valid() const { return _stackSize > 0; }
};

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item, size_t qty = 1);

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ServerItem::vect_t &vect, const ItemSet &itemSet);

const ServerItem *toServerItem(const Item *item);

#endif
