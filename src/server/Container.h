#ifndef CONTAINER_H
#define CONTAINER_H

#include "ServerItem.h"

class Container;
class Object;

class ContainerType{
public:
    static ContainerType *WithSlots(size_t slots);
    Container *instantiate(Object &parent) const;
    size_t slots() const { return _slots; }
private:
    size_t _slots;
    ContainerType(size_t slots);
};


class User;

class Container{
public:
    Container(Object &parent);

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

    friend class ContainerType;
};

#endif
