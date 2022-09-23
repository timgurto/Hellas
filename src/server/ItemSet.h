#ifndef ITEM_SET_H
#define ITEM_SET_H

#include <map>
#include <set>
#include <utility>

class Item;

// A collection of Items, with duplicates/quantities allowed.
class ItemSet {
  std::map<const Item *, size_t> _set;
  size_t _totalQty{0};

 public:
  ItemSet() = default;
  ItemSet(const Item *item);

  const ItemSet &operator+=(const ItemSet &rhs);
  const ItemSet &operator+=(const Item *newItem);

  // Subtraction: remove rhs from lhs, with a floor of zero rather than a
  // concept of negatives.
  const ItemSet &operator-=(const ItemSet &rhs);
  size_t operator[](const Item *key) const;
  void clear() {
    _set.clear();
    _totalQty = 0;
  }
  ItemSet operator-(const ItemSet &rhs) const;
  void set(const Item *item, size_t quantity = 1);
  bool contains(const ItemSet &rhs) const;  // Subset
  bool contains(const Item *item, size_t qty = 1) const;
  bool contains(const std::string &tagName);
  bool contains(const std::set<std::string> &tags);
  size_t numTypes() const { return _set.size(); }
  size_t totalQuantity() const { return _totalQty; }
  void add(const Item *item, size_t qty = 1);
  void remove(const Item *item, size_t qty = 1);
  void remove(const ItemSet &rhs);
  bool isEmpty() const { return _set.empty(); }

  template <typename T>
  std::set<const T *> itemsOnly() const {
    auto items = std::set<const T *>{};
    for (auto pair : _set) {
      const auto *item = dynamic_cast<const T *>(pair.first);
      items.insert(item);
    }
    return items;
  }

  void checkTotalQty() const;

  // Wrappers for inner
  std::map<const Item *, size_t>::const_iterator begin() const {
    return _set.begin();
  }
  std::map<const Item *, size_t>::const_iterator end() const {
    return _set.end();
  }
};

bool operator<=(const ItemSet &lhs, const ItemSet &rhs);
bool operator>(const ItemSet &lhs, const ItemSet &rhs);

#endif
