// (C) 2015 Tim Gurto

#ifndef SERVER_ITEM_H
#define SERVER_ITEM_H

#include <SDL.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../Item.h"

class ObjectType;

// Describes an item type
class ServerItem : public Item{
    std::string _id; // The no-space, unique name used in data files
    size_t _stackSize;
    std::set<std::string> _classes;

    // The object that this item can construct
    const ObjectType *_constructsObject;

public:
    ServerItem(const std::string &id);

    bool operator<(const ServerItem &rhs) const { return _id < rhs._id; }
    
    typedef std::vector<std::pair<const ServerItem *, size_t> > vect_t;

    const std::string &id() const { return _id; }
    size_t stackSize() const { return _stackSize; }
    void stackSize(size_t n) { _stackSize = n; }
    void icon(const std::string &filename);
    const std::set<std::string> &classes() const { return _classes; }
    void constructsObject(const ObjectType *obj) { _constructsObject = obj; }
    const ObjectType *constructsObject() const { return _constructsObject; }

    typedef std::vector<std::pair<const ServerItem *, size_t> > vect_t;

    void addClass(const std::string &className);
    bool hasClasses() const { return _classes.size() > 0; }
    bool isClass(const std::string &className) const;
};

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item, size_t qty = 1);

#endif
