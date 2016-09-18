// (C) 2015 Tim Gurto

#ifndef ITEM_SET_H
#define ITEM_SET_H

#include <map>
#include <set>
#include <utility>
#include "ServerItem.h"

// A collection of Items, with duplicates/quantities allowed.
class ItemSet{
    std::map<const ServerItem *, size_t> _set;
    size_t _totalQty;

public:
    ItemSet();

    const ItemSet &operator+=(const ItemSet &rhs);
    const ItemSet &operator+=(const ServerItem *newItem);

    // Subtraction: remove rhs from lhs, with a floor of zero rather than a concept of negatives.
    const ItemSet &operator-=(const ItemSet &rhs);
    size_t operator[](const ServerItem *key) const;

    void set(const ServerItem *item, size_t quantity = 1);
    bool contains(const ItemSet &rhs) const; // Subset
    bool contains(const ServerItem *item, size_t qty = 1) const;
    bool contains(const std::string &className);
    bool contains(const std::set<std::string> &classes);
    size_t totalQuantity() const { return _totalQty; }
    void add(const ServerItem *item, size_t qty = 1);
    void remove(const ServerItem *item, size_t qty = 1);
    bool isEmpty() const { return _set.empty(); }

    void checkTotalQty() const;

    // Wrappers for inner 
    std::map<const ServerItem *, size_t>::const_iterator begin() const { return _set.begin(); }
    std::map<const ServerItem *, size_t>::const_iterator end() const { return _set.end(); }
};

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator<=(const ItemSet &lhs, const ItemSet &rhs);

bool operator>(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ItemSet &lhs, const ItemSet &rhs) ;

#endif
