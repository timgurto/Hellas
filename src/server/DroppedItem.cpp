#include "DroppedItem.h"

#include "Server.h"
#include "User.h"

DroppedItem::Type DroppedItem::TYPE;

DroppedItem::Type::Type() : EntityType("droppedItem") {
  _baseStats.maxHealth = 1;
}

DroppedItem::DroppedItem(const ServerItem &itemType, Hitpoints health,
                         size_t quantity, std::string suffix,
                         const MapPoint &location)
    : Entity(&TYPE, location),
      _quantity(quantity),
      _itemHealth(health),
      _itemType(itemType),
      _suffix(suffix) {
  // Once-off init
  if (!TYPE.collides()) TYPE.collisionRect({-8, -8, 16, 16});
}

void DroppedItem::sendInfoToClient(const User &targetUser, bool isNew) const {
  auto isNewArg = isNew ? "1"s : "0"s;
  targetUser.sendMessage(
      {SV_DROPPED_ITEM_INFO,
       makeArgs(serial(), location().x, location().y, _itemType.id(), _quantity,
                _itemHealth, _suffix, isNewArg)});
}

void DroppedItem::getPickedUpBy(User &user) {
  auto remainder = user.giveItem(&_itemType, _quantity, _itemHealth, _suffix);

  if (remainder == _quantity) {
    user.sendMessage({WARNING_INVENTORY_FULL});
    return;
  }

  if (remainder > 0) {
    user.sendMessage({WARNING_INVENTORY_FULL});
    Server::instance().addEntity(new DroppedItem(
        _itemType, _itemHealth, remainder, _suffix, location()));
  }

  markForRemoval();
}

void DroppedItem::addToItemCounts(Server::ItemCounts &itemCounts) const {
  itemCounts[_itemType.id()] += _quantity;
}
