#include "ServerItem.h"

#include "Server.h"
#include "User.h"
#include "objects/ObjectType.h"

ServerItem::ServerItem(const std::string &idArg) : Item(idArg) {}

bool ServerItem::canBeDamaged() const {
  if (hasTags()) return true;  // Tools can always be damaged
  if (!isGear()) return false;
  if (_weaponAmmo == this) return false;  // Thrown weapon; consumes itself

  return true;
}

bool ServerItem::isGear() const { return _gearSlot != Item::NOT_GEAR; }

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
    auto itemInSlot = slot.first.type();
    auto qtyInSlot = slot.second;

    if (!itemInSlot) {
      if (qty <= item->stackSize()) return true;
      qty -= item->stackSize();
    } else if (itemInSlot == item) {
      auto roomInSlot = item->stackSize() - qtyInSlot;
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
      const auto itemInSlot = slot.first.type();
      auto &qtyInSlot = slot.second;

      if (itemInSlot != itemToAdd) continue;

      auto roomInSlot = itemToAdd->stackSize() - qtyInSlot;
      if (qtyToAdd <= roomInSlot) {
        qtyInSlot += qtyToAdd;
        qtyToAdd = 0;
        break;  // All of the item fits into this slot
      }
      qtyToAdd -= roomInSlot;
    }
    if (qtyToAdd == 0) continue;

    // Try to add to empty slots
    for (auto &slot : vect) {
      if (slot.first.hasItem()) continue;

      if (qtyToAdd <= itemToAdd->stackSize()) {
        slot.first = ServerItem::Instance{
            itemToAdd, ServerItem::Instance::ReportingInfo::DummyUser()};
        slot.second = qtyToAdd;
        qtyToAdd = 0;
        break;  // All of this item fits into this empty slot
      }
      slot.first = ServerItem::Instance{
          itemToAdd, ServerItem::Instance::ReportingInfo::DummyUser()};
      slot.second = itemToAdd->stackSize();
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
    if (slot.first.type() != itemThatWillBeRemoved) continue;
    if (slot.second > qtyLeft) {
      slot.first = {};
      slot.second = 0;
      break;
    }
    qtyLeft -= slot.second;
    slot.first = {};
    slot.second = 0;
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
    if (slot.first.isBroken()) {
      brokenItemWasFound = true;
      continue;
    }
    if (slot.first.isSoulbound()) {
      soulboundItemWasFound = true;
      continue;
    }
    remaining.remove(slot.first.type(), slot.second);
    if (remaining.isEmpty()) return ServerItem::ITEMS_PRESENT;
  }
  if (soulboundItemWasFound) return ServerItem::ITEMS_SOULBOUND;
  if (brokenItemWasFound) return ServerItem::ITEMS_BROKEN;
  return ServerItem::ITEMS_MISSING;
}

const ServerItem *toServerItem(const Item *item) {
  return dynamic_cast<const ServerItem *>(item);
}

ServerItem::Instance::Instance(const ServerItem *type, ReportingInfo info)
    : _type(type), _reportingInfo(info) {
  if (!type) return;

  _health = MAX_HEALTH;

  _statsFromSuffix =
      Server::instance()._suffixSets.chooseRandomSuffix(_type->_suffixSet);
}

ServerItem::Instance::Instance(const ServerItem *type, ReportingInfo info,
                               Hitpoints health)
    : _type(type), _reportingInfo(info), _health(health) {
  _statsFromSuffix =
      Server::instance()._suffixSets.chooseRandomSuffix(_type->_suffixSet);
}

void ServerItem::Instance::swap(std::pair<ServerItem::Instance, size_t> &lhs,
                                std::pair<ServerItem::Instance, size_t> &rhs) {
  auto temp = lhs;
  lhs = rhs;
  rhs = temp;

  // Unswap reporting info, since reporting info describes the location of the
  // item rather than the item itself.
  auto tempReportingInfo = lhs.first._reportingInfo;
  lhs.first._reportingInfo = rhs.first._reportingInfo;
  rhs.first._reportingInfo = tempReportingInfo;
}

bool ServerItem::Instance::isBroken() const { return _health == 0; }

void ServerItem::Instance::damageFromUse() {
  --_health;
  _reportingInfo.report();
}

void ServerItem::Instance::damageOnPlayerDeath() {
  const auto DAMAGE_ON_DEATH = Hitpoints{10};
  if (DAMAGE_ON_DEATH >= _health)
    _health = 0;
  else
    _health -= DAMAGE_ON_DEATH;

  _reportingInfo.report();
}

void ServerItem::Instance::repair() {
  _health = MAX_HEALTH;
  _reportingInfo.report();
}

double ServerItem::Instance::toolSpeed(const std::string &tag) const {
  const auto &tags = _type->tags();
  auto it = tags.find(tag);
  if (it == tags.end()) return 1.0;

  return it->second;
}

bool ServerItem::Instance::isSoulbound() const {
  if (!_type) return false;
  if (_type->bindsOnPickup()) return true;
  if (_type->bindsOnEquip()) return _hasBeenEquipped;
  return false;
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
