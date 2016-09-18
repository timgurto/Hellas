// (C) 2015 Tim Gurto

#ifndef CLIENT_ITEM_H
#define CLIENT_ITEM_H

#include <map>
#include <string>
#include <set>
#include <vector>

#include "Texture.h"
#include "../Item.h"

class ClientObjectType;

// The client-side representation of an item type
class ClientItem : public Item{
    std::string _id;
    std::string _name;
    Texture _icon;

    std::set<std::string> _classes;

    // The object that this item can construct
    const ClientObjectType *_constructsObject;

public:
    ClientItem(const std::string &id, const std::string &name = "");

    const std::string &name() const { return _name; }
    const Texture &icon() const { return _icon; }

    typedef std::vector<std::pair<const ClientItem *, size_t> > vect_t;
    
    bool operator<(const ClientItem &rhs) const { return _id < rhs._id; }

    const std::string &id() const { return _id; }
    void icon(const std::string &filename);
    const std::set<std::string> &classes() const { return _classes; }
    void constructsObject(const ClientObjectType *obj) { _constructsObject = obj; }
    const ClientObjectType *constructsObject() const { return _constructsObject; }

    void addClass(const std::string &className);
    bool hasClasses() const { return _classes.size() > 0; }
    bool isClass(const std::string &className) const;
};

#endif
