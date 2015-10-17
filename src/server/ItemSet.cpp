// (C) 2015 Tim Gurto

#include "ItemSet.h"
#include "../util.h"

ItemSet::ItemSet():
_totalQty(0){}

const ItemSet &ItemSet::operator+=(const ItemSet &rhs){
    for (const auto &pair : rhs) {
        _set[pair.first] += pair.second;
        _totalQty += pair.second;
    }
    return *this;
}

const ItemSet &ItemSet::operator+=(const Item *newItem){
    ++_set[newItem];
    ++_totalQty;
    return *this;
}

const ItemSet &ItemSet::operator-=(const ItemSet &rhs){
    for (const auto &pair : rhs) {
        auto it = _set.find(pair.first);
        size_t qtyToRemove = min(pair.second, it->second);
        it->second -= qtyToRemove;
        _totalQty -= qtyToRemove;
        if (it->second == 0)
            _set.erase(it);

    }
    return *this;
}

//ItemSet ItemSet::operator-(const ItemSet &rhs) const{
//    
//}

size_t ItemSet::operator[](const Item *key) const{
    auto it = _set.find(key);
    if (it == _set.end())
        return 0;
    return it->second;
}

void ItemSet::set(const Item *item, size_t quantity){
    size_t oldQty = _set[item];
    auto it = _set.find(item);
    it->second = quantity;
    if (quantity == 0)
        _set.erase(it);
    _totalQty = _totalQty - oldQty + quantity;
}

bool ItemSet::contains(const ItemSet &rhs) const{
    ItemSet remaining = *this;
    for (const auto &pair : rhs) {
        auto it = remaining._set.find(pair.first);
        if (it == _set.end())
            return false;
        if (it->second < pair.second)
            return false;
        it->second -= pair.second;
        if (it->second == 0)
            remaining._set.erase(it);
    }
    return true;
}

void ItemSet::add(const Item *item, size_t qty){
    if (qty == 0)
        return;
    _set[item] += qty;
    _totalQty += qty;
}

void ItemSet::remove(const Item *item, size_t qty){
    if (qty == 0)
        return;
    auto it = _set.find(item);
    if (it == _set.end())
        return;
    size_t qtyToRemove = min(qty, it->second);
    it->second -= qtyToRemove;
    _totalQty -= qtyToRemove;
}
