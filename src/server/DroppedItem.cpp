#include "DroppedItem.h"

#include "User.h"

DroppedItem::Type DroppedItem::commonType;

DroppedItem::DroppedItem(const ServerItem& itemType)
    : Entity(&commonType, MapPoint({})), _itemType(itemType) {}

void DroppedItem::sendInfoToClient(const User& targetUser) const {
  targetUser.sendMessage({SV_DROPPED_ITEM, _itemType.id()});
}
