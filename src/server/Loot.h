#ifndef LOOT_H
#define LOOT_H

#include "ServerItem.h"

class User;

// Describes the specific loot available when an NPC dies
class Loot {
 public:
  bool empty() const;
  void add(const ServerItem *item, size_t qty);
  void add(const ItemSet &items);
  void sendSingleSlotToUser(const User &recipient, Serial serial,
                            size_t slot) const;
  bool isValidSlot(size_t slot) const { return _container.size() > slot; }
  ServerItem::Instance &at(size_t i) { return _container[i]; }
  size_t size() const { return _container.size(); }

 private:
  ServerItem::vect_t _container;
};

#endif
