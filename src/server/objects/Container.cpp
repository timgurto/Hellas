#include "Container.h"

#include "../../util.h"
#include "../Server.h"
#include "../User.h"
#include "Object.h"

ContainerType *ContainerType::WithSlots(size_t slots) {
  return new ContainerType(slots);
}

ContainerType::ContainerType(size_t slots) : _slots(slots) {}

Container *ContainerType::instantiate(Object &parent) const {
  Container *p = new Container(parent);
  p->_container = ServerItem::vect_t(_slots, ServerItem::Instance{});
  return p;
}

Container::Container(Object &parent) : _parent(parent) {}

bool Container::isEmpty() const {
  for (auto &item : _container)
    if (item.hasItem()) return false;
  return true;
}

void Container::removeItems(const ItemSet &items) {
  std::set<size_t> invSlotsChanged;
  ItemSet remaining = items;
  for (size_t i = 0; i != _container.size(); ++i) {
    auto &invSlot = _container[i];
    if (remaining.contains(invSlot.type())) {
      size_t itemsToRemove = min(invSlot.quantity(), remaining[invSlot.type()]);
      remaining.remove(invSlot.type(), itemsToRemove);
      invSlot.removeItems(itemsToRemove);
      if (invSlot.quantity() == 0) invSlot = {};
      invSlotsChanged.insert(i);
      if (remaining.isEmpty()) break;
    }
  }
  for (size_t slotNum : invSlotsChanged)
    _parent.tellRelevantUsersAboutInventorySlot(slotNum);
}

void Container::removeAll() {
  for (size_t i = 0; i != _container.size(); ++i) _container[i] = {};
  for (size_t slotNum = 0; slotNum != _container.size(); ++slotNum)
    _parent.tellRelevantUsersAboutInventorySlot(slotNum);
}

void Container::addItems(const ServerItem *item, size_t qty) {
  std::set<size_t> changedSlots;
  // First pass: partial stacks
  for (size_t i = 0; i != _container.size(); ++i) {
    auto &invSlot=_container[i];
    if (invSlot.type() != item) continue;
    size_t spaceAvailable = item->stackSize() - invSlot.quantity();
    if (spaceAvailable > 0) {
      size_t qtyInThisSlot = min(spaceAvailable, qty);
      invSlot.addItems(qtyInThisSlot);
      changedSlots.insert(i);
      qty -= qtyInThisSlot;
    }
    if (qty == 0) break;
    ;
  }

  // Second pass: empty slots
  if (qty != 0)
    for (size_t i = 0; i != _container.size(); ++i) {
      if (_container[i].hasItem()) continue;
      size_t qtyInThisSlot = min(item->stackSize(), qty);
      _container[i] = {item,
                       ServerItem::Instance::ReportingInfo::InObjectContainer(),
                       qtyInThisSlot};
      changedSlots.insert(i);
      qty -= qtyInThisSlot;
      if (qty == 0) break;
      ;
    }

  if (qty > 0)
    SERVER_ERROR("items left over when trying to add to a container");

  for (auto slot : changedSlots)
    _parent.tellRelevantUsersAboutInventorySlot(slot);
}

bool Container::isAbleToDeconstruct(const User &user) const {
  if (!isEmpty()) {
    user.sendMessage(WARNING_NOT_EMPTY);
    return false;
  }
  return true;
}

bool Container::containsAnySoulboundItems() const {
  for (auto &item : _container)
    if (item.isSoulbound()) return true;

  return false;
}

ItemSet Container::generateLootWithChance(double chance) const {
  ItemSet loot;

  for (size_t slot = 0; slot != _container.size(); ++slot) {
    const auto &item = _container[slot];
    if (!item.type()) continue;
    size_t qty = 0;
    auto numInSlot = item.quantity();
    for (size_t i = 0; i != numInSlot; ++i) {
      if (randDouble() < chance) ++qty;
    }
    if (qty > 0) loot.add(item.type(), qty);
  }

  return loot;
}
