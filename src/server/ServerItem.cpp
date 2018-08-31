#include "ServerItem.h"
#include "Server.h"
#include "objects/ObjectType.h"

ServerItem::ServerItem(const std::string &idArg)
    : Item(idArg), _constructsObject(nullptr) {}

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
    auto itemInSlot = slot.first;
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

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect) {
  ItemSet remaining = itemSet;
  for (size_t i = 0; i != vect.size(); ++i) {
    const std::pair<const ServerItem *, size_t> &invSlot = vect[i];
    remaining.remove(invSlot.first, invSlot.second);
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
