#include "CDroppedItem.h"

#include "Client.h"

CDroppedItem::Type CDroppedItem::commonType;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {
  drawRect(ScreenRect{-Client::ICON_SIZE / 2, -Client::ICON_SIZE / 2,
                      Client::ICON_SIZE, Client::ICON_SIZE});
}

CDroppedItem::CDroppedItem(Serial serial, const MapPoint& location,
                           const ClientItem& itemType, size_t quantity,
                           bool isNew)
    : ClientObject(serial, &commonType, location),
      _itemType(itemType),
      _quantity(quantity) {
  if (isNew) {
    _itemType.playSoundOnce("drop");
  }
}

const std::string& CDroppedItem::name() const {
  _name = _itemType.name();
  if (_quantity > 1) _name += " x"s + toString(_quantity);
  return _name;
}

const Texture& CDroppedItem::image() const { return _itemType.icon(); }

const Texture& CDroppedItem::getHighlightImage() const {
  return _itemType.iconHighlighted();
}

const Tooltip& CDroppedItem::tooltip() const {
  return Sprite::tooltip();  // No tooltip
}

void CDroppedItem::onLeftClick(Client& client) {
  Client::instance().sendMessage({CL_PICK_UP_DROPPED_ITEM, serial()});
}

void CDroppedItem::onRightClick(Client& client) {
  Client::instance().sendMessage({CL_PICK_UP_DROPPED_ITEM, serial()});
}
