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
  p->_container =
      ServerItem::vect_t(_slots, std::make_pair(ServerItem::Instance{}, 0));
  return p;
}

Container::Container(Object &parent) : _parent(parent) {}

bool Container::isEmpty() const {
  for (auto pair : _container)
    if (pair.first.hasItem()) return false;
  return true;
}

void Container::removeItems(const ItemSet &items) {
  std::set<size_t> invSlotsChanged;
  ItemSet remaining = items;
  for (size_t i = 0; i != _container.size(); ++i) {
    auto &invSlot = _container[i];
    if (remaining.contains(invSlot.first.type())) {
      size_t itemsToRemove =
          min(invSlot.second, remaining[invSlot.first.type()]);
      remaining.remove(invSlot.first.type(), itemsToRemove);
      _container[i].second -= itemsToRemove;
      if (_container[i].second == 0) _container[i].first = {};
      invSlotsChanged.insert(i);
      if (remaining.isEmpty()) break;
    }
  }
  for (size_t slotNum : invSlotsChanged)
    _parent.tellRelevantUsersAboutInventorySlot(slotNum);
}

void Container::removeAll() {
  for (size_t i = 0; i != _container.size(); ++i) {
    auto &invSlot = _container[i];
    invSlot.first = {};
    invSlot.second = 0;
  }
  for (size_t slotNum = 0; slotNum != _container.size(); ++slotNum)
    _parent.tellRelevantUsersAboutInventorySlot(slotNum);
}

void Container::addItems(const ServerItem *item, size_t qty) {
  std::set<size_t> changedSlots;
  // First pass: partial stacks
  for (size_t i = 0; i != _container.size(); ++i) {
    if (_container[i].first.type() != item) continue;
    size_t spaceAvailable = item->stackSize() - _container[i].second;
    if (spaceAvailable > 0) {
      size_t qtyInThisSlot = min(spaceAvailable, qty);
      _container[i].second += qtyInThisSlot;
      changedSlots.insert(i);
      qty -= qtyInThisSlot;
    }
    if (qty == 0) break;
    ;
  }

  // Second pass: empty slots
  if (qty != 0)
    for (size_t i = 0; i != _container.size(); ++i) {
      if (_container[i].first.hasItem()) continue;
      size_t qtyInThisSlot = min(item->stackSize(), qty);
      _container[i].first = {
          item, ServerItem::Instance::ReportingInfo::InObjectContainer()};
      _container[i].second = qtyInThisSlot;
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
  for (auto pair : _container) {
    if (pair.first.isSoulbound()) return true;
  }
  return false;
}

ItemSet Container::generateLootWithChance(double chance) const {
  ItemSet loot;

  for (size_t slot = 0; slot != _container.size(); ++slot) {
    const auto *pItem = _container[slot].first.type();
    if (pItem == nullptr) continue;
    size_t qty = 0;
    auto numInSlot = _container[slot].second;
    for (size_t i = 0; i != numInSlot; ++i) {
      if (randDouble() < chance) ++qty;
    }
    if (qty > 0) loot.add(pItem, qty);
  }

  return loot;
}
