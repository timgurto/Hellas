// (C) 2015 Tim Gurto

#ifndef OBJECT_H
#define OBJECT_H

#include "MerchantSlot.h"
#include "ObjectType.h"
#include "ItemSet.h"
#include "../Point.h"

// A server-side representation of an in-game object
class Object{
    size_t _serial; // Starts at 1; 0 is reserved.
    Point _location;
    const ObjectType *_type;
    std::string _owner;
    ItemSet _contents; // Remaining contents, which can be gathered
    Item::vect_t _container; // Items contained in object
    std::vector<MerchantSlot> _merchantSlots;

protected:
    static size_t generateSerial();

public:
    // Both constructors generate new serials
    Object(const ObjectType *type, const Point &loc);
    Object(size_t serial); // For set/map lookup; contains only a serial

    const Point &location() const { return _location; }
    size_t serial() const { return _serial; }
    void serial(size_t s) { _serial = s; }
    const ObjectType *type() const { return _type; }
    const std::string &owner() const { return _owner; }
    void owner(const std::string &name) { _owner = name; }
    const ItemSet &contents() const { return _contents; }
    void contents(const ItemSet &contents);
    Item::vect_t &container() { return _container; }
    const Item::vect_t &container() const { return _container; }
    const Rect collisionRect() const { return _type->collisionRect() + _location;  }
    const std::vector<MerchantSlot> &merchantSlots() const { return _merchantSlots; }
    const MerchantSlot &merchantSlot(size_t slot) const { return _merchantSlots[slot]; }
    MerchantSlot &merchantSlot(size_t slot) { return _merchantSlots[slot]; }

    bool operator<(const Object &rhs) const { return _serial < rhs._serial; }

    // Randomly choose an item type for the user to gather.
    const Item *chooseGatherItem() const;
    // Randomly choose a quantity of the above items, between 1 and the object's contents.
    size_t chooseGatherQuantity(const Item *item) const;
    void removeItem(const Item *item, size_t qty);

    bool userHasAccess(const std::string &username) const;
};

#endif
