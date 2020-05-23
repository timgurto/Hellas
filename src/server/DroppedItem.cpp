#include "DroppedItem.h"

#include "User.h"

DroppedItem::Type DroppedItem::commonType;

void DroppedItem::sendInfoToClient(const User& targetUser) const {
  targetUser.sendMessage({SV_DROPPED_ITEM});
}
