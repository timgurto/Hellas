#include "DroppedItem.h"

#include "User.h"

DroppedItem::Type DroppedItem::commonType;

DroppedItem::Type::Type() : EntityType("droppedItem") {
  _baseStats.maxHealth = 1;
}

DroppedItem::DroppedItem(const ServerItem &itemType, size_t quantity,
                         const MapPoint &location)
    : Entity(&commonType, location), _quantity(quantity), _itemType(itemType) {}

void DroppedItem::sendInfoToClient(const User &targetUser) const {
  targetUser.sendMessage(
      {SV_DROPPED_ITEM,
       makeArgs(serial(), location().x, location().y, _itemType.id())});
}

void DroppedItem::giveItemTo(User &user) {
  user.giveItem(&_itemType, _quantity);
}
