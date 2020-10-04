#include "DroppedItem.h"

#include "Server.h"
#include "User.h"

DroppedItem::Type DroppedItem::TYPE;

DroppedItem::Type::Type() : EntityType("droppedItem") {
  _baseStats.maxHealth = 1;
}

DroppedItem::DroppedItem(const ServerItem &itemType, size_t quantity,
                         const MapPoint &location)
    : Entity(&TYPE, location), _quantity(quantity), _itemType(itemType) {
  // Once-off init
  if (!TYPE.collides()) TYPE.collisionRect({-8, -8, 16, 16});
}

void DroppedItem::sendInfoToClient(const User &targetUser, bool isNew) const {
  auto isNewArg = isNew ? "1"s : "0"s;
  targetUser.sendMessage(
      {SV_DROPPED_ITEM, makeArgs(serial(), location().x, location().y,
                                 _itemType.id(), _quantity, isNewArg)});
}

void DroppedItem::getPickedUpBy(User &user) {
  auto remainder = user.giveItem(&_itemType, _quantity);

  if (remainder == _quantity) {
    user.sendMessage({WARNING_INVENTORY_FULL});
    return;
  }

  if (remainder > 0) {
    user.sendMessage({WARNING_INVENTORY_FULL});
    Server::instance().addEntity(
        new DroppedItem(_itemType, remainder, location()));
  }

  markForRemoval();
}
