#include "ItemSet.h"

#include "../Item.h"
#include "../util.h"

ItemSet::ItemSet(const Item *item) { add(item); }

const ItemSet &ItemSet::operator+=(const ItemSet &rhs) {
  for (const auto &pair : rhs) {
    _set[pair.first] += pair.second;
    _totalQty += pair.second;
  }
  checkTotalQty();
  return *this;
}

const ItemSet &ItemSet::operator+=(const Item *newItem) {
  ++_set[newItem];
  ++_totalQty;
  checkTotalQty();
  return *this;
}

const ItemSet &ItemSet::operator-=(const ItemSet &rhs) {
  for (const auto &pair : rhs) {
    auto it = _set.find(pair.first);
    size_t qtyToRemove = min(pair.second, it->second);
    it->second -= qtyToRemove;
    _totalQty -= qtyToRemove;
    if (it->second == 0) _set.erase(it);
  }
  checkTotalQty();
  return *this;
}

size_t ItemSet::operator[](const Item *key) const {
  auto it = _set.find(key);
  if (it == _set.end()) return 0;
  return it->second;
}

ItemSet ItemSet::operator-(const ItemSet &rhs) const {
  auto copy = *this;
  copy.remove(rhs);
  return copy;
}

void ItemSet::set(const Item *item, size_t quantity) {
  size_t oldQty = _set[item];

  // Can't report, as this could be the server or the client.
  // assert(_totalQty >= oldQty);

  auto it = _set.find(item);
  it->second = quantity;
  _totalQty = _totalQty - oldQty + quantity;
  if (quantity == 0) _set.erase(it);
  checkTotalQty();
}

bool ItemSet::contains(const ItemSet &rhs) const {
  ItemSet remaining = *this;
  for (const auto &pair : rhs) {
    auto it = remaining._set.find(pair.first);
    if (it == _set.end()) return false;
    if (it->second < pair.second) return false;
    it->second -= pair.second;
    if (it->second == 0) remaining._set.erase(it);
  }
  return true;
}

bool ItemSet::contains(const Item *item, size_t qty) const {
  if (qty == 0) return true;
  auto it = _set.find(item);
  if (it == _set.end()) return false;
  return (it->second >= qty);
}

bool ItemSet::contains(const std::string &tagName) {
  for (auto pair : _set)
    if (pair.first->tags().find(tagName) != pair.first->tags().end())
      return true;
  return false;
}

bool ItemSet::contains(const std::set<std::string> &tags) {
  for (const std::string &name : tags)
    if (!contains(name)) return false;
  return true;
}

void ItemSet::add(const Item *item, size_t qty) {
  if (qty == 0) return;
  _set[item] += qty;
  _totalQty += qty;
  checkTotalQty();
}

void ItemSet::remove(const Item *item, size_t qty) {
  if (qty == 0) return;
  auto it = _set.find(item);
  if (it == _set.end()) return;
  size_t qtyToRemove = min(qty, it->second);
  it->second -= qtyToRemove;
  if (it->second == 0) _set.erase(it);
  _totalQty -= qtyToRemove;
  checkTotalQty();
}

void ItemSet::remove(const ItemSet &rhs) {
  for (const auto &pair : rhs) remove(pair.first, pair.second);
}

void ItemSet::checkTotalQty() const {
  size_t total = 0;
  for (const auto &pair : _set) total += pair.second;

  // Can't report, as this could be the server or the client.
  // assert(_totalQty == total);
};

bool operator<=(const ItemSet &lhs, const ItemSet &rhs) {
  return rhs.contains(lhs);
}

bool operator>(const ItemSet &lhs, const ItemSet &rhs) {
  return !(lhs <= rhs);
};
