#include "Client.h"
#include "Texture.h"
#include "ui/Picture.h"
#include "ui/Window.h"

void Client::onMapScrollUp(Element &e) {
  instance().zoomMapIn();
  updateMapWindow(e);
}
void Client::onMapScrollDown(Element &e) {
  instance().zoomMapOut();
  updateMapWindow(e);
}

void Client::initializeMapWindow() {
  _mapImage = {"Images/map.png"};
  _mapWindow = Window::WithRectAndTitle(
      {(SCREEN_X - MAP_IMAGE_W) / 2, (SCREEN_Y - MAP_IMAGE_H) / 2,
       MAP_IMAGE_W + 1, MAP_IMAGE_H + 1},
      "Map");
  _mapPicture =
      new Picture(ScreenRect{0, 0, MAP_IMAGE_W, MAP_IMAGE_H}, _mapImage);
  _mapWindow->addChild(_mapPicture);

  _mapPinOutlines = new Element({0, 0, MAP_IMAGE_W, MAP_IMAGE_H});
  _mapPins = new Element({0, 0, MAP_IMAGE_W, MAP_IMAGE_H});
  _mapWindow->addChild(_mapPinOutlines);
  _mapWindow->addChild(_mapPins);

  // Zoom buttons
  static const auto ZOOM_BUTTON_SIZE = 11;
  _zoomMapInButton = new Button(
      {MAP_IMAGE_W - ZOOM_BUTTON_SIZE, 0, ZOOM_BUTTON_SIZE, ZOOM_BUTTON_SIZE},
      "+", [this]() { zoomMapIn(); });
  _mapWindow->addChild(_zoomMapInButton);
  _zoomMapOutButton = new Button({MAP_IMAGE_W - ZOOM_BUTTON_SIZE * 2, 0,
                                  ZOOM_BUTTON_SIZE, ZOOM_BUTTON_SIZE},
                                 "-", [this]() { zoomMapOut(); });
  _mapWindow->addChild(_zoomMapOutButton);

  _mapWindow->setScrollUpFunction(onMapScrollUp);
  _mapWindow->setScrollDownFunction(onMapScrollDown);

  _mapWindow->setPreRefreshFunction(updateMapWindow);
}

void Client::updateMapWindow(Element &) {
  Client &client = *Client::_instance;
  auto zoomMultiplier = 1 << client._zoom;

  // Unit: point from far top/left to far bottom/right [0,1]
  auto charPosX = client._character.location().x / (client._mapX * TILE_W);
  auto charPosY = client._character.location().y / (client._mapY * TILE_H);
  auto mapDisplacementX = 0.5 - charPosX * zoomMultiplier;
  auto mapDisplacementY = 0.5 - charPosY * zoomMultiplier;

  auto mapDisplacement = ScreenPoint{toInt(mapDisplacementX * MAP_IMAGE_W),
                                     toInt(mapDisplacementY * MAP_IMAGE_H)};

  // Make sure map always fills the screen
  auto xLim = -MAP_IMAGE_W * zoomMultiplier + MAP_IMAGE_W;
  auto yLim = -MAP_IMAGE_H * zoomMultiplier + MAP_IMAGE_H;
  mapDisplacement.x = max(xLim, min(mapDisplacement.x, 0));
  mapDisplacement.y = max(yLim, min(mapDisplacement.y, 0));

  auto picRect = ScreenRect{};
  picRect.x = mapDisplacement.x;
  picRect.y = mapDisplacement.y;
  picRect.w = MAP_IMAGE_W * (1 << client._zoom);
  picRect.h = MAP_IMAGE_H * (1 << client._zoom);
  client._mapPicture->rect(picRect);

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

  client._zoomMapInButton->setEnabled(client._zoom < Client::MAX_ZOOM);
  client._zoomMapOutButton->setEnabled(client._zoom > Client::MIN_ZOOM);
}

void Client::addMapPin(const MapPoint &worldPosition, const Color &color) {
  static const ScreenRect PIN_RECT(0, 0, 1, 1), OUTLINE_RECT(-1, -1, 3, 3);

  auto mapPosition = convertToMapPosition(worldPosition);

  _mapPins->addChild(new ColorBlock(PIN_RECT + mapPosition, color));
  _mapPinOutlines->addChild(
      new ColorBlock(OUTLINE_RECT + mapPosition, Color::UI_OUTLINE));
}

void Client::addOutlinedMapPin(const MapPoint &worldPosition,
                               const Color &color) {
  static const ScreenRect PIN_RECT(0, 0, 1, 1), OUTLINE_RECT_H(-2, -1, 5, 3),
      OUTLINE_RECT_V(-1, -2, 3, 5), BORDER_RECT_H(-1, 0, 3, 1),
      BORDER_RECT_V(0, -1, 1, 3);

  auto mapPosition = convertToMapPosition(worldPosition);

  _mapPins->addChild(
      new ColorBlock(BORDER_RECT_H + mapPosition, Color::UI_OUTLINE_HIGHLIGHT));
  _mapPins->addChild(
      new ColorBlock(BORDER_RECT_V + mapPosition, Color::UI_OUTLINE_HIGHLIGHT));
  _mapPins->addChild(new ColorBlock(PIN_RECT + mapPosition, color));
  _mapPinOutlines->addChild(
      new ColorBlock(OUTLINE_RECT_H + mapPosition, Color::UI_OUTLINE));
  _mapPinOutlines->addChild(
      new ColorBlock(OUTLINE_RECT_V + mapPosition, Color::UI_OUTLINE));
}

ScreenRect Client::convertToMapPosition(const MapPoint &worldPosition) const {
  const double MAP_FACTOR_X = 1.0 * _mapX * TILE_W / MAP_IMAGE_W,
               MAP_FACTOR_Y = 1.0 * _mapY * TILE_H / MAP_IMAGE_H;

  px_t x = toInt(worldPosition.x / MAP_FACTOR_X),
       y = toInt(worldPosition.y / MAP_FACTOR_Y);

  return {x, y, 0, 0};
}

void Client::zoomMapIn() { _zoom = min(_zoom + 1, MAX_ZOOM); }

void Client::zoomMapOut() { _zoom = max(_zoom - 1, MIN_ZOOM); }
