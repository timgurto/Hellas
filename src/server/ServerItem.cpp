#include "ServerItem.h"

#include "Server.h"
#include "User.h"
#include "objects/ObjectType.h"

ServerItem::ServerItem(const std::string &idArg) : Item(idArg) {}

bool ServerItem::canBeRepaired() const {
  if (!_class) return false;
  return _class->repairing.canBeRepaired;
}

std::string ServerItem::randomSuffixFromSet() const {
  if (_suffixSet.empty()) return {};
  const auto &suffix =
      Server::instance()._suffixSets.chooseRandomSuffix(_suffixSet);
  return suffix.id;
}

void ServerItem::fetchAmmoItem() const {
  if (_weaponAmmoID.empty()) return;

  const auto &server = Server::instance();
  auto it = server._items.find(_weaponAmmoID);
  if (it == server._items.end()) {
    server.debug() << Color::CHAT_ERROR << "Unknown item "s << _weaponAmmoID
                   << " specified as ammo"s << Log::endl;
    return;
  }
  _weaponAmmo = &*it;
}

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item,
                  size_t qty) {
  for (const auto &slot : vect) {
    auto itemInSlot = slot.type();

    if (!itemInSlot) {
      if (qty <= item->stackSize()) return true;
      qty -= item->stackSize();
    } else if (itemInSlot == item) {
      auto roomInSlot = item->stackSize() - slot.quantity();
      if (qty <= roomInSlot) return true;
      qty -= roomInSlot;
    } else
      continue;
  }
  return false;
}

bool vectHasSpace(ServerItem::vect_t vect, ItemSet items) {
  for (auto pair : items) {
    auto itemToAdd = dynamic_cast<const ServerItem *>(pair.first);
    auto &qtyToAdd = pair.second;

    // Try to top up slots already containing that item
    for (auto &slot : vect) {
      const auto *itemInSlot = slot.type();

      if (itemInSlot != itemToAdd) continue;

      auto roomInSlot = itemToAdd->stackSize() - slot.quantity();
      if (qtyToAdd <= roomInSlot) {
        slot.addItems(qtyToAdd);
        qtyToAdd = 0;
        break;  // All of the item fits into this slot
      }
      qtyToAdd -= roomInSlot;
    }
    if (qtyToAdd == 0) continue;

    // Try to add to empty slots
    for (auto &slot : vect) {
      if (slot.hasItem()) continue;

      if (qtyToAdd <= itemToAdd->stackSize()) {
        slot = ServerItem::Instance{
            itemToAdd, ServerItem::Instance::ReportingInfo::DummyUser(),
            qtyToAdd};
        qtyToAdd = 0;
        break;  // All of this item fits into this empty slot
      }
      slot = ServerItem::Instance{
          itemToAdd, ServerItem::Instance::ReportingInfo::DummyUser(),
          itemToAdd->stackSize()};
      qtyToAdd -= itemToAdd->stackSize();
    }

    if (qtyToAdd > 0) return false;
  }

  return true;
}

bool vectHasSpaceAfterRemovingItems(const ServerItem::vect_t &vect,
                                    const ServerItem *item, size_t qty,
                                    const ServerItem *itemThatWillBeRemoved,
                                    size_t qtyThatWillBeRemoved) {
  // Make copy
  auto v = vect;

  // Remove items from copy
  auto qtyLeft = qtyThatWillBeRemoved;
  for (auto &slot : v) {
    if (slot.type() != itemThatWillBeRemoved) continue;
    if (slot.quantity() > qtyLeft) {
      slot = {};
      break;
    }
    qtyLeft -= slot.quantity();
    slot = {};
  }

  // Call normal function on copy
  return vectHasSpace(v, item, qty);
}

ServerItem::ContainerCheckResult containerHasEnoughToTrade(
    const ServerItem::vect_t &container, const ItemSet &items) {
  auto remaining = items;
  auto soulboundItemWasFound = false;
  auto brokenItemWasFound = false;
  for (const auto &slot : container) {
    if (!slot.hasItem()) continue;
    if (slot.isBroken()) {
      brokenItemWasFound = true;
      continue;
    }
    if (slot.isSoulbound()) {
      soulboundItemWasFound = true;
      continue;
    }
    remaining.remove(slot.type(), slot.quantity());
    if (remaining.isEmpty()) return ServerItem::ITEMS_PRESENT;
  }
  if (soulboundItemWasFound) return ServerItem::ITEMS_SOULBOUND;
  if (brokenItemWasFound) return ServerItem::ITEMS_BROKEN;
  return ServerItem::ITEMS_MISSING;
}

const ServerItem *toServerItem(const Item *item) {
  return dynamic_cast<const ServerItem *>(item);
}

ServerItem::Instance::Instance(const ServerItem *type, ReportingInfo info,
                               size_t quantity)
    : _type(type), _reportingInfo(info), _quantity(quantity) {
  if (!type) return;

  _health = _type->maxHealth();

  if (!_type->_suffixSet.empty()) {
    const auto &suffix =
        Server::instance()._suffixSets.chooseRandomSuffix(_type->_suffixSet);
    _suffix = suffix.id;
    _statsFromSuffix = suffix.stats;
  }
}

ServerItem::Instance::Instance(const ServerItem *type, ReportingInfo info,
                               Hitpoints health, std::string suffix,
                               size_t quantity)
    : _type(type),
      _reportingInfo(info),
      _health(health),
      _suffix(suffix),
      _quantity(quantity) {
  setSuffixStatsBasedOnSelectedSuffix();
}

void ServerItem::Instance::swap(ServerItem::Instance &lhs,
                                ServerItem::Instance &rhs) {
  auto temp = lhs;
  lhs = rhs;
  rhs = temp;

  // Unswap reporting info, since reporting info describes the location of the
  // item rather than the item itself.
  auto tempReportingInfo = lhs._reportingInfo;
  lhs._reportingInfo = rhs._reportingInfo;
  rhs._reportingInfo = tempReportingInfo;
}

bool ServerItem::Instance::isAtFullHealth() const {
  return _health == _type->maxHealth();
}

bool ServerItem::Instance::isDamaged() const { return !isAtFullHealth(); }

bool ServerItem::Instance::isBroken() const { return _health == 0; }

bool ServerItem::Instance::isSoulbound() const {
  if (!_type) return false;
  if (_type->bindsOnPickup()) return true;
  if (_type->bindsOnEquip()) return _hasBeenEquipped;
  return false;
}

void ServerItem::Instance::setSuffixStatsBasedOnSelectedSuffix() {
  if (_type->_suffixSet.empty()) return;

  _statsFromSuffix = Server::instance()._suffixSets.getStatsForSuffix(
      _type->_suffixSet, _suffix);
}

void ServerItem::Instance::setSuffix(std::string suffixID) {
  _suffix = suffixID;
}

ServerItem::Instance::ReportingInfo
ServerItem::Instance::ReportingInfo::UserGear(const User *owner, size_t slot) {
  return {owner, Serial::Gear(), slot};
}

ServerItem::Instance::ReportingInfo
ServerItem::Instance::ReportingInfo::UserInventory(const User *owner,
                                                   size_t slot) {
  return {owner, Serial::Inventory(), slot};
}

void ServerItem::Instance::ReportingInfo::report() {
  if (!_owner) return;

  if (_container.isInventory())
    _owner->sendInventorySlot(_slot);
  else if (_container.isGear())
    _owner->sendGearSlot(_slot);
  else
    SERVER_ERROR("Trying to report container inventory; unsupported.");
}
