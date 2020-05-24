#include "CDroppedItem.h"

#include "Client.h"

CDroppedItem::Type CDroppedItem::commonType;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {
  drawRect(ScreenRect{-Client::ICON_SIZE / 2, -Client::ICON_SIZE / 2,
                      Client::ICON_SIZE, Client::ICON_SIZE});
}

CDroppedItem::CDroppedItem(Serial serial, const MapPoint& location,
                           const ClientItem& itemType)
    : ClientObject(serial, &commonType, location), _itemType(itemType) {}

const std::string& CDroppedItem::name() const { return _itemType.name(); }

const Texture& CDroppedItem::image() const { return _itemType.icon(); }

const Texture& CDroppedItem::getHighlightImage() const {
  return _itemType.iconHighlighted();
}

const Tooltip& CDroppedItem::tooltip() const {
  return Sprite::tooltip();  // No tooltip
}
