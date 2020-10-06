#include <cassert>

#include "../TerrainList.h"
#include "Client.h"
#include "ClientNPC.h"
#include "Renderer.h"
#include "drawing.h"
#include "ui/ContainerGrid.h"

extern Args cmdLineArgs;
extern Renderer renderer;

void Client::draw() const {
  if (!_loggedIn || !_loaded) {
    renderer.setDrawColor(Color::BLACK);
    renderer.clear();
    _chatLog->draw();
    renderer.present();
    return;
  }

  // Background
  renderer.setDrawColor(Color::BLACK);
  renderer.clear();

  // Used below by terrain and entities
  _constructionFootprintType = _selectedConstruction;
  if (!_constructionFootprintType && containerGridInUse.item())
    _constructionFootprintType = containerGridInUse.item()->constructsObject();
  if (_constructionFootprintType)
    _constructionFootprintAllowedTerrain =
        TerrainList::findList(_constructionFootprintType->allowedTerrain());
  else
    _constructionFootprintAllowedTerrain = nullptr;

  // Terrain
  size_t xMin = static_cast<size_t>(max<double>(0, -offset().x / Map::TILE_W)),
         xMax = static_cast<size_t>(min<double>(
             _map.width(), 1.0 * (-offset().x + SCREEN_X) / Map::TILE_W + 1.5)),
         yMin = static_cast<size_t>(max<double>(0, -offset().y / Map::TILE_H)),
         yMax = static_cast<size_t>(min<double>(
             _map.height(), (-offset().y + SCREEN_Y) / Map::TILE_H + 1));
  assert(xMin <= xMax);
  assert(yMin <= yMax);
  for (size_t y = yMin; y != yMax; ++y) {
    const px_t yLoc = y * Map::TILE_H + toInt(offset().y);
    for (size_t x = xMin; x != xMax; ++x) {
      px_t xLoc = x * Map::TILE_W + toInt(offset().x);
      if (y % 2 == 1) xLoc -= Map::TILE_W / 2;
      drawTile(x, y, xLoc, yLoc);
    }
  }

  // Sprites, sorted from back to front
  static const px_t DRAW_MARGIN_ABOVE = 400, DRAW_MARGIN_BELOW = 400,
                    DRAW_MARGIN_SIDES = 400;
  const double topY = -offset().y - DRAW_MARGIN_BELOW,
               bottomY = -offset().y + SCREEN_Y + DRAW_MARGIN_ABOVE,
               leftX = -offset().x - DRAW_MARGIN_SIDES,
               rightX = -offset().x + SCREEN_X + DRAW_MARGIN_SIDES;
  // Cull by y
  auto top = _entities.lower_bound(&Sprite::YCoordOnly(topY));
  auto bottom = _entities.upper_bound(&Sprite::YCoordOnly(bottomY));
  auto visibleSprites = SpritesToDraw{*this};
  visibleSprites.copy(top, bottom);
  visibleSprites.cullHorizontally(leftX, rightX);

  visibleSprites.drawConstructionSiteFootprints();

  // Base under target combatant
  if (_target.exists()) {
    const Texture &base =
        _target.isAggressive() ? images.baseAggressive : images.basePassive;
    static const ScreenPoint BASE_OFFSET(-15, -10);
    base.draw(toScreenPoint(_target.entity()->location()) + offset() +
              BASE_OFFSET);
  }

  visibleSprites.drawFlatSprites();
  visibleSprites.drawNonFlatSprites();

  if (isDebug()) visibleSprites.drawDrawOrder();

  if (_constructionFootprint) visibleSprites.drawCollisionScene();

  visibleSprites.drawNamesAndHealthbars();

  // Character's server location
  if (isDebug()) {
    renderer.setDrawColor(Color::YELLOW);
    const ScreenPoint &serverLoc =
        toScreenPoint(_character.locationOnServer()) + offset();
    renderer.drawRect({serverLoc.x - 1, serverLoc.y - 1, 3, 3});
  }

  // Non-window UI, back-to-front (normal order is front-to-back for input
  // capturing)
  for (std::list<Element *>::const_reverse_iterator it = _ui.rbegin();
       it != _ui.rend(); ++it) {
    auto &e = **it;

    auto willCaptureMouseEvents = e.visible() && !e.isIgnoringMouseEvents();
    if (isDebug() && willCaptureMouseEvents) {
      renderer.setDrawColor(Color::BLUE);
      renderer.fillRect(e.rect());
    }

    e.draw();
  }

  // Windows
  for (windows_t::const_reverse_iterator it = _windows.rbegin();
       it != _windows.rend(); ++it)
    (*it)->draw();

  // Dragged item
  static const ScreenPoint MOUSE_ICON_OFFSET(-Client::ICON_SIZE / 2,
                                             -Client::ICON_SIZE / 2);
  const auto *draggedItem = containerGridBeingDraggedFrom.item();
  if (draggedItem) draggedItem->icon().draw(_mouse + MOUSE_ICON_OFFSET);

  // Construction footprint
  _instructionsLabel->changeText({});
  if (_constructionFootprint && _constructionFootprintType) {
    auto footprintRect = _constructionFootprintType->collisionRect() +
                         toMapPoint(_mouse) - _offset;
    auto validLocation = true;

    if (distance(playerCollisionRect(), footprintRect) >
        Client::ACTION_DISTANCE)
      validLocation = false;
    auto footprintColor =
        validLocation ? Color::FOOTPRINT_GOOD : Color::FOOTPRINT_BAD;
    drawFootprint(footprintRect, footprintColor, 0xaf);

    const ScreenRect &drawRect = _constructionFootprintType->drawRect();
    px_t x = toInt(_mouse.x + drawRect.x), y = toInt(_mouse.y + drawRect.y);
    _constructionFootprint.setAlpha(0x7f);
    _constructionFootprint.draw(x, y);
    _constructionFootprint.setAlpha();

    auto isInCity = !_character.cityName().empty();
    auto instruction = "Click to build "s + _constructionFootprintType->name() +
                       "; right-click to cancel."s;
    if (isCtrlPressed()) {
      if (isInCity)
        instruction += "  It will be owned by your city."s;
      else
        instruction += "  (You are not in a city)"s;
    } else if (isInCity) {
      instruction += "  Hold Ctrl to build an object for your city.";
    }
    _instructionsLabel->changeText(instruction);
  }

  // Cull distance
  if (isDebug()) {
    const ScreenPoint midScreen =
        toScreenPoint(_character.location()) + offset();
    renderer.setDrawColor(Color::RED);
    renderer.drawRect({midScreen.x - CULL_DISTANCE, midScreen.y - CULL_DISTANCE,
                       CULL_DISTANCE * 2, CULL_DISTANCE * 2});
  }

  // Tooltip
  drawTooltip();

  // Cursor
  _currentCursor->draw(_mouse);

  renderer.present();
  _drawingFinished = true;
}

void Client::drawTooltip() const {
  const Tooltip *tooltip = nullptr;
  if (Element::tooltip() != nullptr)
    tooltip = Element::tooltip();
  else if (_currentMouseOverEntity != nullptr && !_mouseOverWindow) {
    tooltip = &_currentMouseOverEntity->tooltip();
  } else if (_terrainTooltip)
    tooltip = &_terrainTooltip;

  if (tooltip != nullptr) {
    static const px_t EDGE_GAP = 2;     // Gap from screen edges
    static const px_t CURSOR_GAP = 10;  // Horizontal gap from cursor
    px_t x, y;
    const px_t mouseX = toInt(_mouse.x);
    const px_t mouseY = toInt(_mouse.y);

    // y: below cursor, unless too close to the bottom of the screen
    if (SCREEN_Y > mouseY + tooltip->height() + EDGE_GAP)
      y = mouseY;
    else
      y = SCREEN_Y - tooltip->height() - EDGE_GAP;

    // x: to the right of the cursor, unless too close to the right of the
    // screen
    if (SCREEN_X > mouseX + tooltip->width() + EDGE_GAP + CURSOR_GAP)
      x = mouseX + CURSOR_GAP;
    else
      x = mouseX - tooltip->width() - CURSOR_GAP;
    tooltip->draw({x, y});
  }
}

void Client::drawFootprint(const MapRect &rect, Color color,
                           Uint8 alpha) const {
  auto footprint = Texture{toInt(rect.w), toInt(rect.h)};
  renderer.pushRenderTarget(footprint);
  renderer.setDrawColor(color);
  renderer.fill();
  renderer.popRenderTarget();
  footprint.setAlpha(alpha);
  footprint.setBlend();

  footprint.draw(toScreenPoint(_offset + rect));
}

void Client::drawTile(size_t x, size_t y, px_t xLoc, px_t yLoc) const {
  if (isDebug()) {
    gameData.terrain.at(_map[x][y]).draw(xLoc, yLoc);
    return;
  }

  /*
        H | E
    L | tileID| R
        G | F
  */
  const ScreenRect drawLoc(xLoc, yLoc, 0, 0);
  const bool yOdd = (y % 2 == 1);
  char tileID, L, R, E, F, G, H;
  tileID = _map[x][y];
  R = x == _map.width() - 1 ? tileID : _map[x + 1][y];
  L = x == 0 ? tileID : _map[x - 1][y];
  if (y == 0) {
    H = E = tileID;
  } else {
    if (yOdd) {
      E = _map[x][y - 1];
      H = x == 0 ? tileID : _map[x - 1][y - 1];
    } else {
      E = x == _map.width() - 1 ? tileID : _map[x + 1][y - 1];
      H = _map[x][y - 1];
    }
  }
  if (y == _map.height() - 1) {
    G = F = tileID;
  } else {
    if (!yOdd) {
      F = x == _map.width() - 1 ? tileID : _map[x + 1][y + 1];
      G = _map[x][y + 1];
    } else {
      F = _map[x][y + 1];
      G = x == 0 ? tileID : _map[x - 1][y + 1];
    }
  }

  // Handle hard-edged terrain
  const auto *terrainThis = &gameData.terrain.at(tileID);
  if (terrainThis->hasHardEdge()) {
    terrainThis->setFullAlpha();
    terrainThis->draw(xLoc, yLoc);
    terrainThis->setQuarterAlpha();
    return;
  }
  // This tile has soft edges, but for hard-edged neighbours, don't blend.
  const auto *terrainL = &gameData.terrain.at(L),
             *terrainR = &gameData.terrain.at(R),
             *terrainE = &gameData.terrain.at(E),
             *terrainF = &gameData.terrain.at(F),
             *terrainG = &gameData.terrain.at(G),
             *terrainH = &gameData.terrain.at(H);
  if (terrainL->hasHardEdge()) terrainL = terrainThis;
  if (terrainR->hasHardEdge()) terrainR = terrainThis;
  if (terrainE->hasHardEdge()) terrainE = terrainThis;
  if (terrainF->hasHardEdge()) terrainF = terrainThis;
  if (terrainG->hasHardEdge()) terrainG = terrainThis;
  if (terrainH->hasHardEdge()) terrainH = terrainThis;

  const px_t w = Map::TILE_W, h = Map::TILE_H;
  const ScreenRect TOP_LEFT(0, 0, w / 2, h / 2),
      TOP_RIGHT(w / 2, 0, w / 2, h / 2), BOTTOM_LEFT(0, h / 2, w / 2, h / 2),
      BOTTOM_RIGHT(w / 2, h / 2, w / 2, h / 2), LEFT_HALF(0, 0, w / 2, h),
      RIGHT_HALF(w / 2, 0, w / 2, h), FULL(0, 0, w, h);

  auto drawRect = ScreenRect{};
  if (yOdd && x == 0)
    drawRect = drawLoc + RIGHT_HALF;
  else if (!yOdd && x == _map.width() - 1)
    drawRect = drawLoc + LEFT_HALF;
  else
    drawRect = drawLoc + FULL;

  // Black background
  // Assuming all tile images are set to SDL_BLENDMODE_ADD and quarter alpha
  renderer.setDrawColor(Color::BLACK);
  renderer.fillRect(drawRect);

  // Half-alpha base tile
  terrainThis->setHalfAlpha();
  if (yOdd && x == 0) {
    terrainThis->draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
    terrainThis->draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
  } else if (!yOdd && x == _map.width() - 1) {
    terrainThis->draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
    terrainThis->draw(drawLoc + TOP_LEFT, TOP_LEFT);
  } else {
    terrainThis->draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
    terrainThis->draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
    terrainThis->draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
    terrainThis->draw(drawLoc + TOP_LEFT, TOP_LEFT);
  }
  terrainThis->setQuarterAlpha();

  // Quarter-alpha L, R, E, F, G, H tiles
  if (!yOdd || x != 0) {
    terrainL->draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
    terrainL->draw(drawLoc + TOP_LEFT, TOP_LEFT);
    terrainG->draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
    terrainH->draw(drawLoc + TOP_LEFT, TOP_LEFT);
  }
  if (yOdd || x != _map.width() - 1) {
    terrainR->draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
    terrainR->draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
    terrainE->draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
    terrainF->draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
  }

  /*if (!gameData.terrain[tileID].isTraversable()) {
      renderer.setDrawColor(Color::TODO);
      renderer.drawRect(drawLoc + FULL);
  }*/

  // Colour tiles that can't accommodate the selected construction
  if (_constructionFootprintAllowedTerrain &&
      !_constructionFootprintAllowedTerrain->allows(tileID))
    drawFootprint(toMapRect(drawRect) - _offset, Color::FOOTPRINT_COLLISION,
                  0xaf);
}

void Client::drawLoadingScreen(const std::string &msg) const {
  if (cmdLineArgs.contains("hideLoadingScreen")) return;

  static const Color BACKGROUND = Color::WINDOW_BACKGROUND,
                     FOREGROUND = Color::WINDOW_FONT;

  ++_loadingScreenProgress;
  static const auto MAX_PROGRESS = 9;
  auto progress = 1.0 * _loadingScreenProgress / MAX_PROGRESS;

  renderer.setDrawColor(BACKGROUND);
  renderer.clear();

  Texture mainText(_defaultFont, "LOADING", FOREGROUND);
  Texture message(_defaultFont, msg + " . . .", FOREGROUND);

  const px_t Y_MAIN = 160, Y_MSG = 180, Y_BAR = 195, BAR_LENGTH = 80,
             BAR_HEIGHT = 5, X_BAR = (SCREEN_X - BAR_LENGTH) / 2;
  const px_t X_MAIN = (SCREEN_X - mainText.width()) / 2;

  mainText.draw(X_MAIN, Y_MAIN);
  message.draw((SCREEN_X - message.width()) / 2, Y_MSG);
  renderer.setDrawColor(FOREGROUND);
  renderer.drawRect({X_BAR, Y_BAR, BAR_LENGTH, BAR_HEIGHT});
  renderer.fillRect({X_BAR, Y_BAR, toInt(BAR_LENGTH * progress), BAR_HEIGHT});

  renderer.present();
}

void SpritesToDraw::copy(Sprites::const_iterator begin,
                         Sprites::const_iterator end) {
  _container = {begin, end};
}

void SpritesToDraw::cullHorizontally(double leftLimit, double rightLimit) {
  for (auto it = _container.begin(); it != _container.end();) {
    auto x = (*it)->location().x;
    if (x < leftLimit || x > rightLimit)
      it = _container.erase(it);
    else
      ++it;
  }
}

void SpritesToDraw::drawConstructionSiteFootprints() {
  renderer.setDrawColor(Color::FOOTPRINT_ACTIVE);

  for (const auto *sprite : _container) {
    auto pObj = dynamic_cast<const ClientObject *>(sprite);
    if (!pObj) continue;
    if (!pObj->isBeingConstructed()) continue;

    _client.drawFootprint(pObj->collisionRect(), Color::FOOTPRINT_ACTIVE, 0x7f);
  }
}

void SpritesToDraw::drawFlatSprites() {
  for (const auto *sprite : _container) {
    if (!sprite->isFlat()) continue;

    sprite->draw();
  }
}

void SpritesToDraw::drawNonFlatSprites() {
  for (const auto *sprite : _container) {
    if (sprite->isFlat()) continue;

    sprite->draw();
  }
}

void SpritesToDraw::drawDrawOrder() {
  auto drawOrder = 0;

  for (const auto *sprite : _container) {
    ++drawOrder;
    auto number =
        Texture{_client.defaultFont(), toString(drawOrder), Color::MAGENTA};
    number.draw(toScreenPoint(sprite->location()) + _client.offset());
  }
}

void SpritesToDraw::drawCollisionScene() {
  for (const auto *sprite : _container) {
    const auto *obj = dynamic_cast<const ClientObject *>(sprite);
    if (!obj) continue;
    if (!obj->obstructsConstruction()) continue;

    const auto alpha = 0xaf;
    _client.drawFootprint(obj->collisionRect(), Color::FOOTPRINT_COLLISION,
                          alpha);
  }
  // TODO: include dropped items, which will obstruct construction.
}

void SpritesToDraw::drawNamesAndHealthbars() {
  for (const auto *sprite : _container) {
    if (sprite->shouldDrawName()) sprite->drawName();

    const auto *asCombatant = dynamic_cast<const ClientCombatant *>(sprite);
    if (!asCombatant) continue;
    asCombatant->drawHealthBarIfAppropriate(sprite->location(),
                                            sprite->height());
  }
}
