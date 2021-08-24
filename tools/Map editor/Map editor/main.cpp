#include "main.h"

#include <SDL.h>

#include "../../../src/Args.h"
#include "../../../src/XmlReader.h"
#include "../../../src/client/Renderer.h"
#include "../../../src/client/ui/Button.h"
#include "../../../src/client/ui/CheckBox.h"
#include "../../../src/client/ui/ChoiceList.h"
#include "../../../src/client/ui/Line.h"
#include "../../../src/client/ui/List.h"
#include "../../../src/client/ui/OutlinedLabel.h"
#include "../../../src/client/ui/TextBox.h"
#include "../../../src/client/ui/Window.h"
#include "EntityType.h"
#include "Map.h"
#include "SpawnPoint.h"
#include "StaticObject.h"
#include "Terrain.h"
#include "util.h"

auto cmdLineArgs = Args{};   // MUST be defined before renderer
auto renderer = Renderer{};  // MUST be defined after cmdLineArgs

// Circumvent link errors
std::map<std::string, CompositeStat> Stats::compositeDefinitions;

auto loop = true;
auto map = EMap{};
auto zoomLevel = 1;
std::pair<int, int> offset = {0, 0};
auto mouse = ScreenPoint{};
ScreenPoint contextTile;
MapPoint mapPos{};

auto dataFiles = FilesList{};

auto terrain = TerrainType::Container{};
auto staticObjects = StaticObject::Container{};
auto spawnPoints = SpawnPoint::Container{};
auto entityTypes = EntityType::Container{};
auto npcTemplates = NPCTemplate::Container{};

Label *contextPixelLabel{nullptr};
Label *contextTileLabel{nullptr};
Label *contextTerrainLabel{nullptr};
ChoiceList *terrainList{nullptr};
ChoiceList *spawnList{nullptr};

auto playerSpawn = MapPoint{};
auto playerSpawnRange = 0;
auto postTutorialPlayerSpawn = MapPoint{};

// Options
auto shouldDrawSpawnPointCircles = false;
auto shouldScaleStaticImages = false;
auto shouldDrawNPCs = true;
auto shouldDrawObjects = true;
auto shouldSnapToTerrain = true;

auto mouseLeftIsDown = false;
auto terrainToDraw = 'a';

auto windows = std::list<Window *>{};

Window *saveLoadWindow{nullptr};
Window *optionsWindow{nullptr};
Window *contextWindow{nullptr};
Window *terrainWindow{nullptr};
Window *spawnWindow{nullptr};

TextBox *spawnRadiusBox{nullptr};
TextBox *spawnQuantityBox{nullptr};
TextBox *spawnTimeBox{nullptr};

Window *activeTool{nullptr};

TextEntryManager textEntryManager;

Client *noClient{nullptr};

#undef main
int main(int argc, char *argv[]) {
  cmdLineArgs.add("width", "2950");
  cmdLineArgs.add("height", "2000");

  renderer.init();
  SDL_RenderSetLogicalSize(renderer.raw(), renderer.width(), renderer.height());

  dataFiles = findDataFiles("../../Data");
  for (const auto &file : dataFiles) TerrainType::load(terrain, file);
  for (const auto &file : dataFiles) StaticObject::load(staticObjects, file);
  for (const auto &file : dataFiles) SpawnPoint::load(spawnPoints, file);
  for (const auto &file : dataFiles) NPCTemplate::load(npcTemplates, file);
  for (const auto &file : dataFiles)
    EntityType::load(entityTypes, npcTemplates, file);

  Element::font(TTF_OpenFont("trebucbd.ttf", 15));
  Element::textOffset = 2;
  Element::TEXT_HEIGHT = 20;
  Element::initialize();

  initUI();

  map = {"../../Data/map.xml", playerSpawn, playerSpawnRange,
         postTutorialPlayerSpawn};
  offset.first = (map.width() - renderer.width()) / 2;
  offset.second = (map.height() - renderer.height()) / 2;

  auto time = SDL_GetTicks();
  while (loop) {
    auto newTime = SDL_GetTicks();
    auto timeElapsed = newTime - time;
    time = newTime;

    handleInput(timeElapsed);
    render();
  }

  for (auto window : windows) delete window;

  TTF_CloseFont(Element::font());

  return 0;
}

void handleInput(unsigned timeElapsed) {
  auto e = SDL_Event{};
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        loop = false;
        break;

      case SDL_TEXTINPUT:
        TextBox::addText(textEntryManager, e.text.text);
        break;

      case SDL_WINDOWEVENT: {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
          renderer.updateSize();
          for (auto window : windows) window->forceRefresh();
          // Tooltip::forceAllToRedraw();
      } break;

      case SDL_MOUSEMOTION:
        SDL_GetMouseState(&mouse.x, &mouse.y);

        for (Window *window : windows)
          if (window->visible()) window->onMouseMove(mouse);

        if (mouseLeftIsDown) map.set(contextTile.x, contextTile.y);

        {
          mapPos = MapPoint{zoomed(1.0 * mouse.x) + offset.first,
                            zoomed(1.0 * mouse.y) + offset.second} *
                   16.0;
          contextPixelLabel->changeText("Pixel: (" + toString(mapPos.x) + "," +
                                        toString(mapPos.y) + ")");
          auto tile = MapPoint{mapPos.x / 32.0, mapPos.y / 32.0};
          tile.y -= 0.5;
          if (toInt(tile.y) % 2 == 0) tile.x -= 0.5;
          contextTile = toScreenPoint(tile);
          contextTileLabel->changeText("Tile: (" + toString(contextTile.x) +
                                       "," + toString(contextTile.y) + ")");
          auto terrainIndex = map.at(contextTile.x, contextTile.y);
          auto terrainType = terrain[terrainIndex];
          contextTerrainLabel->changeText("Terrain: " + terrainType.id);
        }
        break;

      case SDL_KEYDOWN:

        if (SDL_IsTextInputActive()) {
          // Text input

          switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
              textEntryManager.textBoxInFocus = nullptr;
              SDL_StopTextInput();
              break;

            case SDLK_BACKSPACE:
              TextBox::backspace(textEntryManager);
              break;
          }

        } else {
          switch (e.key.keysym.sym) {
            case SDLK_UP:
              pan(UP);
              break;
            case SDLK_DOWN:
              pan(DOWN);
              break;
            case SDLK_LEFT:
              pan(LEFT);
              break;
            case SDLK_RIGHT:
              pan(RIGHT);
              break;
          }
        }
        break;

      case SDL_MOUSEWHEEL:
        if (e.wheel.y < 0) {
          auto windowScrolled = false;
          for (auto window : windows)
            if (collision(mouse, window->rect())) {
              window->onScrollDown(mouse);
              windowScrolled = true;
              break;
            }
          if (!windowScrolled) zoomOut();

        } else if (e.wheel.y > 0) {
          auto windowScrolled = false;
          for (auto window : windows)
            if (collision(mouse, window->rect())) {
              window->onScrollUp(mouse);
              windowScrolled = true;
              break;
            }
          if (!windowScrolled) zoomIn();
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
        switch (e.button.button) {
          case SDL_BUTTON_LEFT: {
            textEntryManager.textBoxInFocus = nullptr;

            for (auto window : windows)
              if (window->visible()) window->onLeftMouseDown(mouse);

            // Bring top clicked window to front
            auto windowWasClicked = false;
            for (auto *window : windows) {
              if (window->visible() && collision(mouse, window->rect())) {
                windows.remove(window);
                windows.push_front(window);
                window->show();
                windowWasClicked = true;
                break;
              }
            }

            if (SDL_IsTextInputActive() && !textEntryManager.textBoxInFocus)
              SDL_StopTextInput();
            else if (!SDL_IsTextInputActive() &&
                     textEntryManager.textBoxInFocus)
              SDL_StartTextInput();

            if (windowWasClicked) break;

            if (mouse.y != 0) {
              mouseLeftIsDown = true;

              if (activeTool == terrainWindow) {
                auto *state = SDL_GetKeyboardState(nullptr);
                auto shiftIsDown =
                    state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
                if (shiftIsDown)
                  map.fill(contextTile.x, contextTile.y);
                else
                  map.set(contextTile.x, contextTile.y);

              } else if (activeTool == spawnWindow) {
                const auto &selectedTypeID = spawnList->getSelected();
                if (selectedTypeID.empty()) break;

                auto quantity = spawnQuantityBox->textAsNum();
                auto radius = spawnRadiusBox->textAsNum();
                auto respawnTime = spawnTimeBox->textAsNum() * 1000;

                auto sp = SpawnPoint{};
                sp.id = selectedTypeID;
                sp.quantity = quantity;
                sp.radius = radius;
                sp.respawnTime = respawnTime;
                sp.loc = mapPos;
                if (shouldSnapToTerrain) sp.loc = snapToTerrain(sp.loc);

                spawnPoints.insert(sp);
              }
            }
          } break;

          case SDL_BUTTON_RIGHT:
            for (auto window : windows)
              if (window->visible() && collision(mouse, window->rect()))
                window->onRightMouseDown(mouse);
            break;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        switch (e.button.button) {
          case SDL_BUTTON_LEFT: {
            mouseLeftIsDown = false;

            for (auto window : windows)
              if (window->visible() && collision(mouse, window->rect())) {
                window->onLeftMouseUp(mouse);
                break;
              }
          } break;
          case SDL_BUTTON_RIGHT:
            for (auto window : windows)
              if (window->visible() && collision(mouse, window->rect())) {
                window->onRightMouseUp(mouse);
                break;
              }
            break;
        }
        break;
    }
  }

  /*auto keyboardState = SDL_GetKeyboardState(0);
  if (keyboardState[SDL_SCANCODE_UP] == SDL_PRESSED) {
    offset.second -= timeElapsed * zoomLevel;
    render();
  }
  if (keyboardState[SDL_SCANCODE_DOWN] == SDL_PRESSED) {
    offset.second += timeElapsed * zoomLevel;
    render();
  }*/
}

void render() {
  enforcePanLimits();

  const auto BLUE_HELL = Color{24, 82, 161};
  renderer.setDrawColor(BLUE_HELL);
  renderer.clear();

  map.draw(offset);

  drawCircleOnMap(playerSpawn, Color::WHITE, playerSpawnRange);
  drawTextOnMap(playerSpawn, Color::WHITE, "New-player spawn");

  // Draw footprint of selected object to place
  if (activeTool == spawnWindow) {
    const auto &selectedTypeID = spawnList->getSelected();
    if (!selectedTypeID.empty()) {
      auto &entityType = entityTypes[selectedTypeID];
      auto loc = mapPos;
      if (shouldSnapToTerrain) loc = snapToTerrain(loc);
      drawRectOnMap(loc, Color::YELLOW, entityType.collisionRect);
    }
  }

  if (shouldDrawSpawnPointCircles)
    for (const auto &sp : spawnPoints) {
      auto &entityType = entityTypes[sp.id];
      if (entityType.category == EntityType::OBJECT && !shouldDrawObjects)
        continue;
      if (entityType.category == EntityType::NPC && !shouldDrawNPCs) continue;
      auto color =
          entityType.category == EntityType::OBJECT ? Color::CYAN : Color::RED;
      drawCircleOnMap(sp.loc, color, sp.radius);
    }
  for (const auto &so : staticObjects) {
    auto &entityType = entityTypes[so.id];
    drawRectOnMap(so.loc, Color::YELLOW, entityType.collisionRect);
  }
  for (const auto &sp : spawnPoints) {
    auto &entityType = entityTypes[sp.id];
    if (entityType.category == EntityType::OBJECT && !shouldDrawObjects)
      continue;
    if (entityType.category == EntityType::NPC && !shouldDrawNPCs) continue;
    drawImageOnMap(sp.loc, entityType.image, entityType.drawRect);
  }
  for (const auto &so : staticObjects) {
    auto &entityType = entityTypes[so.id];
    drawImageOnMap(so.loc, entityType.image, entityType.drawRect);
  }

  for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
    contextWindow->show();
    auto &window = **it;
    if (window.visible()) {
      if (&window == activeTool) {
        renderer.setDrawColor(Color::YELLOW);
        renderer.fillRect(window.rect() + ScreenRect{-4, -4, 8, 8});
      }
      window.draw();
    }
  }

  renderer.present();
}

ScreenPoint transform(MapPoint mp) {
  mp.x = unzoomed(mp.x / 16.0 - offset.first);
  mp.y = unzoomed(mp.y / 16.0 - offset.second);
  return toScreenPoint({mp.x, mp.y});
}

void drawPointOnMap(const MapPoint &mapLoc, Color color) {
  renderer.setDrawColor(color);
  auto screenPoint = transform(mapLoc);
  const int WEIGHT = 5;
  renderer.fillRect(
      {screenPoint.x - WEIGHT / 2, screenPoint.y - WEIGHT / 2, WEIGHT, WEIGHT});
}

void drawTextOnMap(const MapPoint &mapLoc, Color color,
                   const std::string &text) {
  renderer.setDrawColor(color);
  auto screenPoint = transform(mapLoc);
  auto labelTexture = Texture{Element::font(), text, color};
  labelTexture.draw(ScreenRect{screenPoint.x - labelTexture.width() / 2,
                               screenPoint.y - labelTexture.height() / 2,
                               labelTexture.width(), labelTexture.height()});
}

void drawCircleOnMap(const MapPoint &mapLoc, Color color, int radius) {
  renderer.setDrawColor(color);
  auto screenPoint = transform(mapLoc);
  drawCircle(screenPoint, unzoomed(radius / 16));
}

void drawImageOnMap(const MapPoint &mapLoc, const Texture &image,
                    const ScreenRect &drawRect) {
  auto screenPoint = transform(mapLoc);
  auto scaledDrawRect = ScreenRect{
      toInt(unzoomed(drawRect.x / 16.0)), toInt(unzoomed(drawRect.y / 16.0)),
      toInt(unzoomed(drawRect.w / 16.0)), toInt(unzoomed(drawRect.h / 16.0))};
  auto drawRectToUse = shouldScaleStaticImages ? scaledDrawRect : drawRect;
  image.draw(drawRectToUse + screenPoint);
}

void drawRectOnMap(const MapPoint &mapLoc, Color color,
                   const ScreenRect &drawRect) {
  renderer.setDrawColor(color);
  auto screenPoint = transform(mapLoc);
  auto scaledDrawRect = ScreenRect{
      toInt(unzoomed(drawRect.x / 16.0)), toInt(unzoomed(drawRect.y / 16.0)),
      toInt(unzoomed(drawRect.w / 16.0)), toInt(unzoomed(drawRect.h / 16.0))};
  renderer.fillRect(scaledDrawRect + screenPoint);
}

void initUI() {
  // Save/load window
  saveLoadWindow =
      Window::WithRectAndTitle({0, 200, 300, 100}, "Save/Load", mouse);
  windows.push_front(saveLoadWindow);

  const auto GAP = 2_px,
             BUTTON_W = (saveLoadWindow->contentWidth() - 3 * GAP) / 2,
             BUTTON_H = 25, COL2_X = 2 * GAP + BUTTON_W;

  auto y = GAP;

  saveLoadWindow->addChild(new Label({GAP, y, BUTTON_W, Element::TEXT_HEIGHT},
                                     "Load", Element::CENTER_JUSTIFIED));

  saveLoadWindow->addChild(
      new Button({GAP, y, BUTTON_W, BUTTON_H}, "Load map", []() {
        map = {"../../Data/map.xml", playerSpawn, playerSpawnRange,
               postTutorialPlayerSpawn};
      }));
  saveLoadWindow->addChild(
      new Button({COL2_X, y, BUTTON_W, BUTTON_H}, "Save map", []() {
        map.save("../../Data/map.xml", playerSpawn, playerSpawnRange,
                 postTutorialPlayerSpawn);
      }));
  y += BUTTON_H + GAP;

  saveLoadWindow->addChild(
      new Button({GAP, y, BUTTON_W, BUTTON_H}, "Load spawn points", []() {
        spawnPoints.clear();
        for (const auto &file : dataFiles) SpawnPoint::load(spawnPoints, file);
      }));
  saveLoadWindow->addChild(new Button(
      {COL2_X, y, BUTTON_W, BUTTON_H}, "Save spawn points",
      []() { SpawnPoint::save(spawnPoints, "../../Data/spawnPoints.xml"); }));
  y += BUTTON_H + GAP;

  saveLoadWindow->addChild(
      new Button({GAP, y, BUTTON_W, BUTTON_H}, "Load static objects", []() {
        staticObjects.clear();
        for (const auto &file : dataFiles)
          StaticObject::load(staticObjects, file);
      }));

  // Options window
  optionsWindow =
      Window::WithRectAndTitle({0, 175, 200, 100}, "Options", mouse);
  windows.push_front(optionsWindow);
  auto optionsList = new List(
      {0, 0, optionsWindow->contentWidth(), optionsWindow->contentHeight()});
  optionsWindow->addChild(optionsList);
  const auto cbRect =
      ScreenRect{0, 0, optionsList->width(), optionsList->childHeight()};
  optionsList->addChild(new CheckBox(*noClient, cbRect,
                                     shouldDrawSpawnPointCircles,
                                     "Draw spawn-point circles"));
  optionsList->addChild(new CheckBox(*noClient, cbRect, shouldScaleStaticImages,
                                     "Scale static objects"));
  optionsList->addChild(
      new CheckBox(*noClient, cbRect, shouldDrawObjects, "Draw objects"));
  optionsList->addChild(
      new CheckBox(*noClient, cbRect, shouldDrawNPCs, "Draw NPCs"));
  optionsList->addChild(new CheckBox(*noClient, cbRect, shouldSnapToTerrain,
                                     "Snap objects to terrain"));

  // Context window
  contextWindow = Window::WithRectAndTitle({0, 0, 200, 0}, "Context", mouse);
  windows.push_front(contextWindow);
  y = GAP;

  contextPixelLabel = new Label(ScreenRect{0, y, 200, 20}, {});
  contextWindow->addChild(contextPixelLabel);
  y += contextPixelLabel->height() + GAP;

  contextTileLabel = new Label(ScreenRect{0, y, 200, 20}, {});
  contextWindow->addChild(contextTileLabel);
  y += contextTileLabel->height() + GAP;

  contextTerrainLabel = new Label(ScreenRect{0, y, 200, 20}, {});
  contextWindow->addChild(contextTerrainLabel);
  y += contextTerrainLabel->height() + GAP;

  const auto LINE_SIZE = 200 - 2 * GAP;
  contextWindow->addChild(new Line({GAP, y}, LINE_SIZE));
  y += GAP + 2;

  const auto ICON_SIZE = 32_px;
  const auto BUTTON_SIZE = ICON_SIZE + 2;
  // Row 1: normal windows
  {
    auto x = GAP;

    static auto saveLoadIcon = Texture{"icons/saveLoad.png", Color::MAGENTA};
    auto button = new Button({x, y, BUTTON_SIZE, BUTTON_SIZE}, {},
                             [&]() { saveLoadWindow->toggleVisibility(); });
    button->addChild(new Picture({0, 0, ICON_SIZE, ICON_SIZE}, saveLoadIcon));
    contextWindow->addChild(button);
    x += BUTTON_SIZE;

    static auto optionsIcon = Texture{"icons/options.png", Color::MAGENTA};
    button = new Button({x, y, BUTTON_SIZE, BUTTON_SIZE}, {},
                        [&]() { optionsWindow->toggleVisibility(); });
    button->addChild(new Picture({0, 0, ICON_SIZE, ICON_SIZE}, optionsIcon));
    contextWindow->addChild(button);
    x += BUTTON_SIZE;

    y += BUTTON_SIZE + GAP;
  }

  contextWindow->addChild(new Line({GAP, y}, LINE_SIZE));
  y += GAP + 2;

  // Row 2: tools
  {
    auto x = GAP;
    static auto terrainIcon = Texture{"icons/terrain.png", Color::MAGENTA};
    auto button = new Button({x, y, BUTTON_SIZE, BUTTON_SIZE}, {},
                             [&]() { terrainWindow->toggleVisibility(); });
    button->setTooltip("Draw terrain");
    button->addChild(new Picture({0, 0, ICON_SIZE, ICON_SIZE}, terrainIcon));
    contextWindow->addChild(button);
    x += BUTTON_SIZE + GAP;

    static auto spawnIcon = Texture{"icons/spawn.png", Color::MAGENTA};
    button = new Button({x, y, BUTTON_SIZE, BUTTON_SIZE}, {},
                        [&]() { spawnWindow->toggleVisibility(); });
    button->setTooltip("Add spawners");
    button->addChild(new Picture({0, 0, ICON_SIZE, ICON_SIZE}, spawnIcon));
    contextWindow->addChild(button);

    y += BUTTON_SIZE + GAP;
  }

  contextWindow->height(y);

  // Terrain window
  terrainWindow =
      Window::WithRectAndTitle({300, 0, 200, 1700}, "Terrain", mouse);
  terrainWindow->setMouseMoveFunction(
      [](Element &e, const ScreenPoint &r) {
        if (collision(r, {0, 0, e.width(), e.height()}))
          activeTool = terrainWindow;
      },
      nullptr);
  windows.push_front(terrainWindow);
  terrainList = new ChoiceList(
      {0, 0, terrainWindow->contentWidth(), terrainWindow->contentHeight()}, 34,
      *noClient);
  terrainWindow->addChild(terrainList);
  for (auto pair : terrain) {
    auto &t = pair.second;
    auto entry = new Element({});
    entry->id({t.index});
    terrainList->addChild(entry);
    entry->addChild(new Picture({0, 0, 32, 32}, t.image));
    entry->addChild(new Label({36, 0, entry->rect().w, entry->rect().h}, t.id,
                              Element::LEFT_JUSTIFIED,
                              Element::CENTER_JUSTIFIED));
  }

  // Spawn window
  static const auto TEXT_ROW_H = 20;
  spawnWindow =
      Window::WithRectAndTitle({300, 450, 300, 600}, "Spawn Points", mouse);
  spawnWindow->setMouseMoveFunction(
      [](Element &e, const ScreenPoint &r) {
        if (collision(r, {0, 0, e.width(), e.height()}))
          activeTool = spawnWindow;
      },
      nullptr);
  windows.push_front(spawnWindow);
  spawnList =
      new ChoiceList({0, 0, spawnWindow->contentWidth(),
                      spawnWindow->contentHeight() - 3 * TEXT_ROW_H - 5},
                     20, *noClient);
  spawnWindow->addChild(spawnList);
  // Populate
  for (auto type : entityTypes) {
    auto entry = new Label({}, type.first);
    entry->id(type.first);
    spawnList->addChild(entry);
  }
  static const auto RADIUS_Y =
                        spawnWindow->contentHeight() - 3 * TEXT_ROW_H - 5,
                    QUANTITY_Y =
                        spawnWindow->contentHeight() - 2 * TEXT_ROW_H - 5,
                    RESPAWN_TIME_Y =
                        spawnWindow->contentHeight() - 1 * TEXT_ROW_H - 5,
                    TEXT_BOX_X = 120,
                    TEXT_BOX_W = spawnWindow->contentWidth() - TEXT_BOX_X;
  {
    spawnWindow->addChild(new Label(
        {0, RADIUS_Y, spawnWindow->contentWidth(), Element::TEXT_HEIGHT},
        "Radius"));
    spawnRadiusBox = new TextBox(textEntryManager,
                                 {TEXT_BOX_X, RADIUS_Y, TEXT_BOX_W, TEXT_ROW_H},
                                 TextBox::NUMERALS);
    spawnRadiusBox->text("250");
    spawnWindow->addChild(spawnRadiusBox);
  }
  {
    spawnWindow->addChild(new Label(
        {0, QUANTITY_Y, spawnWindow->contentWidth(), Element::TEXT_HEIGHT},
        "Quantity"));
    spawnQuantityBox = new TextBox(
        textEntryManager, {TEXT_BOX_X, QUANTITY_Y, TEXT_BOX_W, TEXT_ROW_H},
        TextBox::NUMERALS);
    spawnQuantityBox->text("5");
    spawnWindow->addChild(spawnQuantityBox);
  }
  {
    spawnWindow->addChild(new Label(
        {0, RESPAWN_TIME_Y, spawnWindow->contentWidth(), Element::TEXT_HEIGHT},
        "Respawn (s)"));
    spawnTimeBox = new TextBox(
        textEntryManager, {TEXT_BOX_X, RESPAWN_TIME_Y, TEXT_BOX_W, TEXT_ROW_H},
        TextBox::NUMERALS);
    spawnTimeBox->text("300");
    spawnWindow->addChild(spawnTimeBox);
  }
}

MapPoint snapToTerrain(const MapPoint &rhs) {
  auto x = toInt(rhs.x);
  auto y = toInt(rhs.y);
  x = x + 8 - (x + 8) % 16;
  y = y + 8 - (y + 8) % 16;
  return {static_cast<double>(x), static_cast<double>(y)};
}
