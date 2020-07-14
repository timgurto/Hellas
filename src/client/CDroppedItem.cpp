#include "CDroppedItem.h"

#include "Client.h"

CDroppedItem::Type CDroppedItem::commonType;
const double CDroppedItem::DROP_HEIGHT = 60.0;
const double CDroppedItem::DROP_ACCELERATION = 20.0;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {
  drawRect(ScreenRect{-Client::ICON_SIZE / 2, -Client::ICON_SIZE / 2,
                      Client::ICON_SIZE, Client::ICON_SIZE});
}

CDroppedItem::CDroppedItem(Serial serial, const MapPoint& location,
                           const ClientItem& itemType, size_t quantity,
                           bool isNew, Client& client)
    : ClientObject(serial, &commonType, location, client),
      _itemType(itemType),
      _quantity(quantity) {
  if (isNew) _altitude = DROP_HEIGHT;
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

void CDroppedItem::onLeftClick() {
  _client.sendMessage({CL_PICK_UP_DROPPED_ITEM, serial()});
}

void CDroppedItem::onRightClick() {
  _client.sendMessage({CL_PICK_UP_DROPPED_ITEM, serial()});
}

void CDroppedItem::update(double delta) {
  if (isFalling()) {
    _fallSpeed += delta * DROP_ACCELERATION;
    _altitude -= _fallSpeed;
    if (_altitude <= 0) {
      _altitude = 0;
      _itemType.playSoundOnce("drop");
    }
  }

  Sprite::update(delta);
}

void CDroppedItem::draw() const {
  if (!isFalling()) {
    Sprite::draw();
    return;
  }

  if (!image()) return;

  drawShadow();

  auto drawRect = this->drawRect() + _client.offset();
  drawRect.y -= toInt(_altitude);
  image().draw(drawRect.x, drawRect.y);
}

bool CDroppedItem::isFlat() const {
  if (isFalling()) return false;
  return true;
}

bool CDroppedItem::isFalling() const { return _altitude > 0; }