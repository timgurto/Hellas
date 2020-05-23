#include "DroppedItem.h"

#include "User.h"

DroppedItem::Type DroppedItem::commonType;

DroppedItem::Type::Type() : EntityType("droppedItem") {
  _baseStats.maxHealth = 1;
}

DroppedItem::DroppedItem(const ServerItem &itemType, const MapPoint &location)
    : Entity(&commonType, location), _itemType(itemType) {}

void DroppedItem::sendInfoToClient(const User& targetUser) const {
  targetUser.sendMessage({SV_DROPPED_ITEM, makeArgs(serial(), _itemType.id())});
}
