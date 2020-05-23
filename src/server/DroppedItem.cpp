#include "DroppedItem.h"

#include "User.h"

DroppedItem::Type DroppedItem::commonType;

DroppedItem::Type::Type() : EntityType("droppedItem") {
  _baseStats.maxHealth=1;
}

DroppedItem::DroppedItem(const ServerItem& itemType)
    : Entity(&commonType, MapPoint({})), _itemType(itemType) {}

void DroppedItem::sendInfoToClient(const User& targetUser) const {
  targetUser.sendMessage({SV_DROPPED_ITEM, makeArgs(serial(), _itemType.id())});
}
