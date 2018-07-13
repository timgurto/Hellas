#include <SDL.h>

#include "../../../src/Args.h"
#include "../../../src/XmlReader.h"
#include "../../../src/client/Renderer.h"
#include "../../../src/client/ui/Button.h"
#include "../../../src/client/ui/CheckBox.h"
#include "../../../src/client/ui/ChoiceList.h"
#include "../../../src/client/ui/List.h"
#include "../../../src/client/ui/OutlinedLabel.h"
#include "../../../src/client/ui/Window.h"

#include "EntityType.h"
#include "Map.h"
#include "SpawnPoint.h"
#include "StaticObject.h"
#include "Terrain.h"
#include "main.h"
#include "util.h"

auto cmdLineArgs = Args{};   // MUST be defined before renderer
auto renderer = Renderer{};  // MUST be defined after cmdLineArgs

auto loop = true;
auto map = Map{};
auto zoomLevel = 1;
std::pair<int, int> offset = {0, 0};
auto mouse = ScreenPoint{};
ScreenPoint contextTile;

auto terrain = TerrainType::Container{};
auto staticObjects = StaticObject::Container{};
auto spawnPoints = SpawnPoint::Container{};
auto entityTypes = EntityType::Container{};

Label *contextPixelLabel{nullptr};
Label *contextTileLabel{nullptr};
Label *contextTerrainLabel{nullptr};

auto playerSpawn = MapPoint{};
auto playerSpawnRange = 0;

// Options
auto shouldDrawSpawnPointCircles = false;
auto shouldScaleStaticImages = false;

auto mouseLeftIsDown = false;
auto terrainToDraw = 'a';

auto windows = std::list<Window *>{};

#undef main
int main(int argc, char *argv[]) {
  renderer.init();
  SDL_RenderSetLogicalSize(renderer.raw(), renderer.width(), renderer.height());

  auto files = findDataFiles("../../Data");
  for (const auto &file : files) {
    TerrainType::load(terrain, file);
    StaticObject::load(staticObjects, file);
    SpawnPoint::load(spawnPoints, file);
    EntityType::load(entityTypes, file);
  }

  Element::font(TTF_OpenFont("trebucbd.ttf", 15));
  Element::textOffset = 2;
  Element::TEXT_HEIGHT = 20;
  Color::ELEMENT_BACKGROUND = Uint32{0x683141};
  Color::ELEMENT_FONT = Uint32{0xE5E5E5};
  Color::ELEMENT_SHADOW_DARK = Uint32{0x330a17};
  Color::ELEMENT_SHADOW_LIGHT = Uint32{0xa57887};
  Element::absMouse = &mouse;
  Element::initialize();

  initUI();

  map = {"../../Data/map.xml", playerSpawn, playerSpawnRange};
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

        if (mouseLeftIsDown)
          map.set(contextTile.x, contextTile.y, terrainToDraw);

        {
          auto mapPos = MapPoint{zoomed(1.0 * mouse.x) + offset.first,
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
        break;

      case SDL_MOUSEWHEEL:
        if (e.wheel.y < 0) {
          zoomOut();
          for (auto window : windows)
            if (collision(mouse, window->rect())) window->onScrollDown(mouse);
        } else if (e.wheel.y > 0) {
          zoomIn();
          for (auto window : windows)
            if (collision(mouse, window->rect())) window->onScrollUp(mouse);
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
        switch (e.button.button) {
          case SDL_BUTTON_LEFT: {
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
            if (windowWasClicked) break;

            mouseLeftIsDown = true;
            map.set(contextTile.x, contextTile.y, terrainToDraw);
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

  if (shouldDrawSpawnPointCircles)
    for (const auto &sp : spawnPoints) {
      auto &entityType = entityTypes[sp.id];
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
    drawImageOnMap(sp.loc, entityType.image, entityType.drawRect);
  }
  for (const auto &so : staticObjects) {
    auto &entityType = entityTypes[so.id];
    drawImageOnMap(so.loc, entityType.image, entityType.drawRect);
  }

  for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
    (*it)->show();
    (*it)->draw();
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
  auto saveLoadWindow = Window::WithRectAndTitle({0, 0, 200, 100}, "Save/Load");
  windows.push_front(saveLoadWindow);

  const auto GAP = 2_px,
             BUTTON_W = (saveLoadWindow->contentWidth() - 3 * GAP) / 2,
             BUTTON_H = 25, COL2_X = 2 * GAP + BUTTON_W;

  auto y = GAP;

  saveLoadWindow->addChild(new Label({GAP, y, BUTTON_W, Element::TEXT_HEIGHT},
                                     "Load", Element::CENTER_JUSTIFIED));

  saveLoadWindow->addChild(
      new Button({GAP, y, BUTTON_W, BUTTON_H}, "Load map", []() {
        map = {"../../Data/map.xml", playerSpawn, playerSpawnRange};
      }));
  saveLoadWindow->addChild(new Button(
      {COL2_X, y, BUTTON_W, BUTTON_H}, "Save map",
      []() { map.save("../../Data/map.xml", playerSpawn, playerSpawnRange); }));

  // Options window
  auto optionsWindow = Window::WithRectAndTitle({0, 125, 200, 100}, "Options");
  windows.push_front(optionsWindow);
  auto optionsList = new List(
      {0, 0, optionsWindow->contentWidth(), optionsWindow->contentHeight()});
  optionsWindow->addChild(optionsList);
  const auto cbRect =
      ScreenRect{0, 0, optionsList->width(), optionsList->childHeight()};
  optionsList->addChild(new CheckBox(cbRect, shouldDrawSpawnPointCircles,
                                     "Draw spawn-point circles"));
  optionsList->addChild(
      new CheckBox(cbRect, shouldScaleStaticImages, "Scale static objects"));

  // Context window
  auto contextWindow = Window::WithRectAndTitle({0, 600, 200, 0}, "Context");
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

  contextWindow->height(y);

  // Terrain window
  auto terrainWindow = Window::WithRectAndTitle({300, 0, 200, 400}, "Terrain");
  windows.push_front(terrainWindow);
  auto terrainList = new ChoiceList(
      {0, 0, terrainWindow->contentWidth(), terrainWindow->contentHeight()},
      34);
  terrainWindow->addChild(terrainList);
  for (auto pair : terrain) {
    auto &t = pair.second;
    auto button = new Button({}, {});
    terrainList->addChild(button);
    button->addChild(new Picture({0, 0, 32, 32}, t.image));
    button->addChild(new Label({36, 0, button->rect().w, button->rect().h},
                               t.id, Element::LEFT_JUSTIFIED,
                               Element::CENTER_JUSTIFIED));
  }
}
