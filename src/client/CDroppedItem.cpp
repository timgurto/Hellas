#include "CDroppedItem.h"

#include "Client.h"

const double CDroppedItem::DROP_HEIGHT = 60.0;
const double CDroppedItem::DROP_ACCELERATION = 20.0;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {
  drawRect(ScreenRect{-Client::ICON_SIZE / 2, -Client::ICON_SIZE,
                      Client::ICON_SIZE, Client::ICON_SIZE});
  auto iconSize = static_cast<double>(Client::ICON_SIZE);
  collisionRect({-iconSize / 2, -iconSize / 4, iconSize, iconSize / 2});
}

CDroppedItem::CDroppedItem(Client& client, Serial serial,
                           const MapPoint& location, const ClientItem& itemType,
                           size_t quantity, Hitpoints health,
                           std::string suffix, bool isNew)
    : ClientObject(serial, &client.droppedItemType, location, client),
      _itemType(itemType),
      _quantity(quantity),
      _health(health),
      _suffix(suffix) {
  if (isNew) _altitude = DROP_HEIGHT;
}

const std::string& CDroppedItem::name() const {
  _name = _itemType.nameWithSuffix(_suffix);
  if (_quantity > 1) _name += " x"s + toString(_quantity);
  return _name;
}

const Texture& CDroppedItem::image() const { return _itemType.icon(); }

const Texture& CDroppedItem::getHighlightImage() const {
  return _itemType.iconHighlighted();
}

const Tooltip& CDroppedItem::tooltip() const {
  if (_tooltip.hasValue()) return _tooltip.value();
  _tooltip = Tooltip{};
  auto& tooltip = _tooltip.value();

  tooltip.embed(_itemType.tooltip(_suffix));

  if (_health < _itemType.maxHealth()) {
    tooltip.addGap();

    if (_health == 0)
      tooltip.setColor(Color::DURABILITY_BROKEN);
    else if (_health <= 20)
      tooltip.setColor(Color::DURABILITY_LOW);

    tooltip.addLine("Durability: "s + toString(_health) + "/"s +
                    toString(_itemType.maxHealth()));
  }

  tooltip.addGap();
  tooltip.setColor(Color::TOOLTIP_BODY);
  tooltip.addLine("Can be picked up by any player");

  return tooltip;
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
      _itemType.playSoundOnce(_client, "drop");
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

bool CDroppedItem::isFlat() const { return false; }

Color CDroppedItem::nameColor() const { return _itemType.nameColor(); }

bool CDroppedItem::isFalling() const { return _altitude > 0; }
