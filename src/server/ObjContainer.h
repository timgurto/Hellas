#ifndef OBJ_CONTAINER_H
#define OBJ_CONTAINER_H

#include "ServerItem.h"

class ObjContainer;
class Object;

class ObjTypeContainer{
public:
    static ObjTypeContainer *WithSlots(size_t slots);
    ObjContainer *instantiate(Object &parent) const;
    size_t slots() const { return _slots; }
private:
    size_t _slots;
    ObjTypeContainer(size_t slots);
};


class User;

class ObjContainer{
public:
    ObjContainer(Object &parent);

    bool isEmpty() const;
    void removeItems(const ItemSet &items);
    void addItems(const ServerItem *item, size_t qty = 1);
    const std::pair<const ServerItem *, size_t> &at(size_t i) const { return _container[i]; }
    std::pair<const ServerItem *, size_t> &at(size_t i) { return _container[i]; }

    bool isAbleToDeconstruct(const User &user) const;
    
    //TODO: remove
    const ServerItem::vect_t &raw() const { return _container; }
    ServerItem::vect_t &raw() { return _container; }
    
private:
    ServerItem::vect_t _container; // Items contained in object
    Object &_parent;

    friend class ObjTypeContainer;
};

#endif
