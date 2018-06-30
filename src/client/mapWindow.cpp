#include "Client.h"
#include "Texture.h"
#include "ui/Picture.h"
#include "ui/Window.h"

void Client::initializeMapWindow() {
  _mapImage = Texture(std::string("Images/map.png"));
  _mapWindow = Window::WithRectAndTitle(
      {(SCREEN_X - _mapImage.width()) / 2, (SCREEN_Y - _mapImage.height()) / 2,
       _mapImage.width(), _mapImage.height()},
      "Map");
  _mapWindow->addChild(new Picture(0, 0, _mapImage));

  _mapPinOutlines = new Element({0, 0, _mapImage.width(), _mapImage.height()});
  _mapPins = new Element({0, 0, _mapImage.width(), _mapImage.height()});
  _mapWindow->addChild(_mapPinOutlines);
  _mapWindow->addChild(_mapPins);

  _mapWindow->setPreRefreshFunction(updateMapWindow);
}

void Client::updateMapWindow(Element &) {
  Client &client = *Client::_instance;

  client._mapPins->clearChildren();
  client._mapPinOutlines->clearChildren();

  for (const auto &objPair : client._objects) {
    const auto &object = *objPair.second;
    client.addMapPin(object.location(), object.nameColor());
  }

  for (const auto &pair : client._otherUsers) {
    const auto &avatar = *pair.second;
    client.addMapPin(avatar.location(), avatar.nameColor());
  }

  client.addOutlinedMapPin(client._character.location(), Color::COMBATANT_SELF);
}

void Client::addMapPin(const MapPoint &worldPosition, const Color &color) {
  static const ScreenRect PIN_RECT(0, 0, 1, 1), OUTLINE_RECT(-1, -1, 3, 3);

  auto mapPosition = convertToMapPosition(worldPosition);

  _mapPins->addChild(new ColorBlock(PIN_RECT + mapPosition, color));
  _mapPinOutlines->addChild(
      new ColorBlock(OUTLINE_RECT + mapPosition, Color::OUTLINE));
}

void Client::addOutlinedMapPin(const MapPoint &worldPosition,
                               const Color &color) {
  static const ScreenRect PIN_RECT(0, 0, 1, 1), OUTLINE_RECT_H(-2, -1, 5, 3),
      OUTLINE_RECT_V(-1, -2, 3, 5), BORDER_RECT_H(-1, 0, 3, 1),
      BORDER_RECT_V(0, -1, 1, 3);

  auto mapPosition = convertToMapPosition(worldPosition);

  _mapPins->addChild(new ColorBlock(BORDER_RECT_H + mapPosition, Color::WHITE));
  _mapPins->addChild(new ColorBlock(BORDER_RECT_V + mapPosition, Color::WHITE));
  _mapPins->addChild(new ColorBlock(PIN_RECT + mapPosition, color));
  _mapPinOutlines->addChild(
      new ColorBlock(OUTLINE_RECT_H + mapPosition, Color::OUTLINE));
  _mapPinOutlines->addChild(
      new ColorBlock(OUTLINE_RECT_V + mapPosition, Color::OUTLINE));
}

ScreenRect Client::convertToMapPosition(const MapPoint &worldPosition) const {
  const double MAP_FACTOR_X = 1.0 * _mapX * TILE_W / _mapImage.width(),
               MAP_FACTOR_Y = 1.0 * _mapY * TILE_H / _mapImage.height();

  px_t x = toInt(worldPosition.x / MAP_FACTOR_X),
       y = toInt(worldPosition.y / MAP_FACTOR_Y);
  return {x, y, 0, 0};
}
