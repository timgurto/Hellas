#include "ServerItem.h"

#include "Server.h"
#include "objects/ObjectType.h"

ServerItem::ServerItem(const std::string &idArg) : Item(idArg) {}

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

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect) {
  ItemSet remaining = itemSet;
  for (const auto &slot : vect) {
    if (slot.first.isBroken()) continue;
    remaining.remove(slot.first.type(), slot.second);
    if (remaining.isEmpty()) return true;
  }
  return false;
}

bool operator>(const ItemSet &itemSet, const ServerItem::vect_t &vect) {
  return !(itemSet <= vect);
}

bool operator>(const ServerItem::vect_t &vect, const ItemSet &itemSet) {
  return !(itemSet <= vect);
}

const ServerItem *toServerItem(const Item *item) {
  return dynamic_cast<const ServerItem *>(item);
}

ServerItem::Instance::Instance(const ServerItem *type, ReportingInfo info)
    : _type(type), _reportingInfo(info) {
  if (type) _health = MAX_HEALTH;
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

ServerItem::Instance::ReportingInfo
ServerItem::Instance::ReportingInfo::UserGear(const User *owner, size_t slot) {
  return {owner, Server::SpecialSerial::GEAR, slot};
}

ServerItem::Instance::ReportingInfo
ServerItem::Instance::ReportingInfo::UserInventory(const User *owner,
                                                   size_t slot) {
  return {owner, Server::SpecialSerial::INVENTORY, slot};
}

void ServerItem::Instance::ReportingInfo::report() {
  if (!_owner) return;

  if (_container == Server::SpecialSerial::INVENTORY)
    _owner->sendInventorySlot(_slot);
  else if (_container == Server::SpecialSerial::GEAR)
    _owner->sendGearSlot(_slot);
  else
    SERVER_ERROR("Trying to report container inventory; unsupported.");
}
