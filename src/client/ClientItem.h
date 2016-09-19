// (C) 2015 Tim Gurto

#ifndef CLIENT_ITEM_H
#define CLIENT_ITEM_H

#include <map>

#include "Texture.h"
#include "../Item.h"

class ClientObjectType;

// The client-side representation of an item type
class ClientItem : public Item{
    std::string _name;
    Texture _icon;

    // The object that this item can construct
    const ClientObjectType *_constructsObject;

public:
    ClientItem(const std::string &id, const std::string &name = "");

    const std::string &name() const { return _name; }
    const Texture &icon() const { return _icon; }

    typedef std::vector<std::pair<const ClientItem *, size_t> > vect_t;

    void icon(const std::string &filename);
    void constructsObject(const ClientObjectType *obj) { _constructsObject = obj; }
    const ClientObjectType *constructsObject() const { return _constructsObject; }
};

const ClientItem *toClientItem(const Item *item);

#endif
