#include "Loot.h"

#include "../util.h"
#include "Server.h"
#include "User.h"

bool Loot::empty() const {
  if (_container.size() == 0) return true;
  for (const auto &pair : _container) {
    auto item = pair.first;
    if (!item.hasItem()) continue;
    size_t quantity = pair.second;
    if (quantity > 0) return false;
  }
  return true;
}

void Loot::add(const ServerItem *item, size_t qty) {
  size_t stackSize = item->stackSize();
  size_t remainingQuantity = qty;
  while (remainingQuantity > 0) {
    size_t quantityInThisSlot = min(stackSize, remainingQuantity);
    ServerItem::Slot entry;
    entry.first = {item,
                   ServerItem::Instance::ReportingInfo::InObjectContainer()};

    if (item->canBeDamaged())
      entry.first.initHealth(0);  // Loot items are always broken

    entry.second = quantityInThisSlot;
    remainingQuantity -= quantityInThisSlot;
    _container.push_back(entry);
  }
}

void Loot::add(const ItemSet &items) {
  for (const auto &pair : items) {
    const auto *pItem = dynamic_cast<const ServerItem *>(pair.first);
    add(pItem, pair.second);
  }
}

void Loot::sendSingleSlotToUser(const User &recipient, Serial serial,
                                size_t slot) const {
  const Server &server = Server::instance();
  server.sendInventoryMessageInner(recipient, serial, slot, _container);
}
