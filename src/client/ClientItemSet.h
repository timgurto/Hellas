// (C) 2015 Tim Gurto

#ifndef CLIENT_ITEM_SET_H
#define CLIENT_ITEM_SET_H

#include <map>
#include <set>
#include <utility>
#include "ClientItem.h"

// A collection of Items, with duplicates/quantities allowed.
class ClientItemSet{
    std::map<const ClientItem *, size_t> _set;
    size_t _totalQty;

public:
    ClientItemSet();

    const ClientItemSet &operator+=(const ClientItemSet &rhs);
    const ClientItemSet &operator+=(const ClientItem *newItem);

    // Subtraction: remove rhs from lhs, with a floor of zero rather than a concept of negatives.
    const ClientItemSet &operator-=(const ClientItemSet &rhs);
    size_t operator[](const ClientItem *key) const;

    void set(const ClientItem *item, size_t quantity = 1);
    bool contains(const ClientItemSet &rhs) const; // Subset
    bool contains(const ClientItem *item, size_t qty = 1) const;
    bool contains(const std::string &className);
    bool contains(const std::set<std::string> &classes);
    size_t totalQuantity() const { return _totalQty; }
    void add(const ClientItem *item, size_t qty = 1);
    void remove(const ClientItem *item, size_t qty = 1);
    bool isEmpty() const { return _set.empty(); }

    void checkTotalQty() const;

    // Wrappers for inner 
    std::map<const ClientItem *, size_t>::const_iterator begin() const { return _set.begin(); }
    std::map<const ClientItem *, size_t>::const_iterator end() const { return _set.end(); }
};

bool operator<=(const ClientItemSet &itemSet, const ClientItem::vect_t &vect);
bool operator<=(const ClientItemSet &lhs, const ClientItemSet &rhs);

bool operator>(const ClientItemSet &itemSet, const ClientItem::vect_t &vect);
bool operator>(const ClientItemSet &lhs, const ClientItemSet &rhs) ;

#endif
