#include "Client.h"
#include "Texture.h"
#include "ui/Picture.h"
#include "ui/Window.h"

extern Renderer renderer;

void Client::onMapScrollUp(Element &e) {
  auto *window = dynamic_cast<Window *>(&e);
  if (!window || !window->client()) return;

  window->client()->zoomMapIn();
  updateMapWindow(e);
}
void Client::onMapScrollDown(Element &e) {
  auto *window = dynamic_cast<Window *>(&e);
  if (!window || !window->client()) return;

  window->client()->zoomMapOut();
  updateMapWindow(e);
}

void Client::initializeMapWindow() {
  _mapWindow = Window::WithRectAndTitle(

      {(SCREEN_X - MAP_IMAGE_W) / 2, (SCREEN_Y - MAP_IMAGE_H) / 2,
       MAP_IMAGE_W + 1, MAP_IMAGE_H + 2},
      "Map", mouse());
  _mapPicture =
      new Picture(ScreenRect{0, 0, MAP_IMAGE_W, MAP_IMAGE_H}, images.map);
  _mapWindow->addChild(_mapPicture);

  mapWindow.fogOfWar = new Picture(0, 0, _fogOfWar);
  _mapWindow->addChild(mapWindow.fogOfWar);

  _mapPinOutlines = new Element({0, 0, MAP_IMAGE_W, MAP_IMAGE_H});
  _mapPins = new Element({0, 0, MAP_IMAGE_W, MAP_IMAGE_H});
  _mapIcons = new Element({0, 0, MAP_IMAGE_W, MAP_IMAGE_H});
  _mapWindow->addChild(_mapPinOutlines);
  _mapWindow->addChild(_mapPins);
  _mapWindow->addChild(_mapIcons);

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

void Client::updateMapWindow(Element &e) {
  const auto *window = dynamic_cast<const Window *>(&e);
  if (!window || !window->client()) return;
  auto &client = *window->client();

  client.mapWindow.zoomMultiplier = 1 << client._zoom;

  // Unit: point from far top/left to far bottom/right [0,1]
  auto charPosX =
      client._character.location().x / (client._map.width() * Map::TILE_W);
  auto charPosY =
      client._character.location().y / (client._map.height() * Map::TILE_H);
  auto mapDisplacementX = 0.5 - charPosX * client.mapWindow.zoomMultiplier;
  auto mapDisplacementY = 0.5 - charPosY * client.mapWindow.zoomMultiplier;

  client.mapWindow.displacement = {toInt(mapDisplacementX * MAP_IMAGE_W),
                                   toInt(mapDisplacementY * MAP_IMAGE_H)};

  // Make sure map always fills the screen
  auto xLim = -MAP_IMAGE_W * client.mapWindow.zoomMultiplier + MAP_IMAGE_W;
  auto yLim = -MAP_IMAGE_H * client.mapWindow.zoomMultiplier + MAP_IMAGE_H;
  client.mapWindow.displacement.x =
      max(xLim, min(client.mapWindow.displacement.x, 0));
  client.mapWindow.displacement.y =
      max(yLim, min(client.mapWindow.displacement.y, 0));

  auto picRect = ScreenRect{};
  picRect.x = client.mapWindow.displacement.x;
  picRect.y = client.mapWindow.displacement.y;
  picRect.w = MAP_IMAGE_W * (1 << client._zoom);
  picRect.h = MAP_IMAGE_H * (1 << client._zoom);
  client._mapPicture->rect(picRect);

  *client.mapWindow.fogOfWar = {0, 0, client._fogOfWar};
  client.mapWindow.fogOfWar->rect(picRect);

  client._mapPins->clearChildren();
  client._mapPinOutlines->clearChildren();
  client._mapIcons->clearChildren();

  for (const auto &objPair : client._objects) {
    const auto &object = *objPair.second;
    if (object.belongsToPlayer() || object.belongsToPlayerCity())
      client.addMapPin(object.location(), object.nameColor(), object.name());
  }

  for (const auto &pair : client._otherUsers) {
    const auto &avatar = *pair.second;
    if (avatar.isInPlayersCity())
      client.addMapPin(avatar.location(), avatar.nameColor(), avatar.name());
  }

  client.addIconToMap(client._respawnPoint, &images.mapRespawn,
                      "On death, you will return here");

  // TODO: STOP HARDCODING THIS
  // Halloween event:
  client.addIconToMap({33000, 33600}, &images.mapRespawn, "Thessaly Graveyard");
  //
  // Anzac Day event:
  // client.addIconToMap({64674, 17774}, &images.mapRespawn, "Gallipolis
  // Memorial");

  // Cities
  for (const auto &pair : client._cities) {
    auto tooltipText = pair.first + " (player city)";
    auto cityIcon = &images.mapCityNeutral;

    auto isInCity = !client.character().cityName().empty();
    if (client.character().cityName() == pair.first) {
      cityIcon = &images.mapCityFriendly;
      tooltipText = "Your city, "s + pair.first;
    } else if (isInCity && client.isCityAtWarWithCityDirectly(pair.first)) {
      cityIcon = &images.mapCityEnemy;
    } else if (!isInCity && client.isAtWarWithCityDirectly(pair.first)) {
      cityIcon = &images.mapCityEnemy;
    }
    client.addIconToMap(pair.second, cityIcon, tooltipText);
  }

  for (const auto &pin : client.gameData.mapPins)
    client.addIconToMap(pin.location, &images.mapCityNeutral, pin.tooltip);

  client.addOutlinedMapPin(client._character.location(), Color::COMBATANT_SELF);

  client._zoomMapInButton->setEnabled(client._zoom < Client::MAX_ZOOM);
  client._zoomMapOutButton->setEnabled(client._zoom > Client::MIN_ZOOM);
}

void Client::addMapPin(const MapPoint &worldPosition, const Color &color,
                       const std::string &tooltip) {
  static const ScreenRect PIN_RECT(0, 0, 1, 1), OUTLINE_RECT(-1, -1, 3, 3);

  auto mapPosition = convertToMapPosition(worldPosition);

  auto pin = new ColorBlock(PIN_RECT + mapPosition, color);
  if (!tooltip.empty()) pin->setTooltip(tooltip);
  _mapPins->addChild(pin);

  auto outline = new ColorBlock(OUTLINE_RECT + mapPosition, Color::UI_OUTLINE);
  if (!tooltip.empty()) outline->setTooltip(tooltip);
  _mapPinOutlines->addChild(outline);
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

void Client::addIconToMap(const MapPoint &worldPosition, const Texture *icon,
                          const std::string &tooltip) {
  if (!icon) return;

  auto mapPosition = convertToMapPosition(worldPosition);
  mapPosition.x -= icon->width() / 2;
  mapPosition.y -= icon->height() / 2;

  auto picture = new Picture(mapPosition.x, mapPosition.y, *icon);
  if (!tooltip.empty()) picture->setTooltip(tooltip);
  _mapIcons->addChild(picture);
}

ScreenRect Client::convertToMapPosition(const MapPoint &worldPosition) const {
  const double MAP_FACTOR_X = 1.0 * _map.width() * Map::TILE_W / MAP_IMAGE_W,
               MAP_FACTOR_Y = 1.0 * _map.height() * Map::TILE_H / MAP_IMAGE_H;

  px_t x = toInt(worldPosition.x / MAP_FACTOR_X * mapWindow.zoomMultiplier),
       y = toInt(worldPosition.y / MAP_FACTOR_Y * mapWindow.zoomMultiplier);

  return mapWindow.displacement + ScreenRect{x, y, 0, 0};
}

void Client::zoomMapIn() { _zoom = min(_zoom + 1, MAX_ZOOM); }

void Client::zoomMapOut() { _zoom = max(_zoom - 1, MIN_ZOOM); }

void Client::clearChunkFromFogOfWar(size_t x, size_t y) {
  auto transparentPixel = Texture{1, 1};
  transparentPixel.setAlpha(0);
  transparentPixel.setBlend(SDL_BLENDMODE_NONE);

  renderer.pushRenderTarget(_fogOfWar);

  transparentPixel.draw(x, y);

  renderer.popRenderTarget();
}

void Client::redrawFogOfWar() {
  if (isDebug()) return;

  _fogOfWar = {_fogOfWar.width(), _fogOfWar.height()};
  _fogOfWar.setBlend();

  auto transparentPixel = Texture{1, 1};
  transparentPixel.setAlpha(0);
  transparentPixel.setBlend(SDL_BLENDMODE_NONE);

  renderer.pushRenderTarget(_fogOfWar);

  renderer.setDrawColor(Color::BLACK);
  renderer.fill();

  for (auto x = 0; x != _fogOfWar.width(); ++x)
    for (auto y = 0; y != _fogOfWar.height(); ++y)
      if (_mapExplored[x][y]) transparentPixel.draw(x, y);

  renderer.popRenderTarget();

  updateMapWindow(Element{});
}
