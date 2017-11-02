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
    size_t _stackSize = 1;

    // The object that this item can construct
    const ObjectType *_constructsObject;

    // An item returned to the user after this is used as a construction material
    const ServerItem *_returnsOnConstruction = nullptr;

public:
    ServerItem(const std::string &id);

    typedef std::pair<const ServerItem *, size_t> Slot;
    typedef std::vector<Slot> vect_t;

    size_t stackSize() const { return _stackSize; }
    void stackSize(size_t n) { _stackSize = n; }
    void constructsObject(const ObjectType *obj) { _constructsObject = obj; }
    const ObjectType *constructsObject() const { return _constructsObject; }
    const ServerItem *returnsOnConstruction() const { return _returnsOnConstruction; }
    void returnsOnConstruction(const ServerItem *item) { _returnsOnConstruction = item; }
    bool valid() const { return _stackSize > 0; }
};

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item, size_t qty = 1);

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ServerItem::vect_t &vect, const ItemSet &itemSet);

const ServerItem *toServerItem(const Item *item);

#endif
