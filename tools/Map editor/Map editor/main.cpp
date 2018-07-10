#include <SDL.h>

#include "../../../src/Args.h"
#include "../../../src/XmlReader.h"
#include "../../../src/client/Renderer.h"
#include "../../../src/client/ui/Button.h"
#include "../../../src/client/ui/OutlinedLabel.h"
#include "../../../src/client/ui/Window.h"

#include "EntityType.h"
#include "Map.h"
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

auto terrain = TerrainType::Container{};
auto staticObjects = StaticObject::Container{};
auto entityTypes = EntityType::Container{};

auto playerSpawn = MapPoint{};
auto playerSpawnRange = 0;

auto windows = std::list<Window *>{};

#undef main
int main(int argc, char *argv[]) {
  renderer.init();
  SDL_RenderSetLogicalSize(renderer.raw(), renderer.width(), renderer.height());

  auto files = findDataFiles("../../Data");
  for (const auto &file : files) {
    TerrainType::load(terrain, file);
    StaticObject::load(staticObjects, file);
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
          case SDL_BUTTON_LEFT:
            for (auto window : windows)
              if (window->visible()) window->onLeftMouseDown(mouse);

            // Bring top clicked window to front
            for (auto *window : windows) {
              if (window->visible() && collision(mouse, window->rect())) {
                windows.remove(window);
                windows.push_front(window);
                window->show();
                break;
              }
            }
            break;
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
            for (auto window : windows)
              if (window->visible() && collision(mouse, window->rect())) {
                window->onLeftMouseUp(mouse);
                break;
              }
          }
          case SDL_BUTTON_RIGHT:
            for (auto window : windows)
              if (window->visible() && collision(mouse, window->rect())) {
                window->onRightMouseUp(mouse);
                break;
              }
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

  auto cursorLabelRect = ScreenRect{0, renderer.height() - 20, 200, 20};
  auto mapPos = MapPoint{zoomed(1.0 * mouse.x) + offset.first,
                         zoomed(1.0 * mouse.y) + offset.second} *
                16.0;
  auto cursorPosLabel =
      OutlinedLabel{cursorLabelRect, "Cursor is at (" + toString(mapPos.x) +
                                         "," + toString(mapPos.y) + ")"};
  cursorPosLabel.draw();

  for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
    (*it)->show();
    (*it)->draw();
  }

  drawCircleOnMap(playerSpawn, Color::WHITE, playerSpawnRange);
  drawTextOnMap(playerSpawn, Color::WHITE, "New-player spawn");

  for (const auto &so : staticObjects) {
    auto &entityType = entityTypes[so.id];
    drawRectOnMap(so.loc, Color::YELLOW, entityType.collisionRect);
  }
  for (const auto &so : staticObjects) {
    auto &entityType = entityTypes[so.id];
    drawImageOnMap(so.loc, entityType.image, entityType.drawRect);
  }

  renderer.present();
}

void pan(Direction dir) {
  const auto PAN_AMOUNT = 200;
  switch (dir) {
    case UP:
      offset.second -= zoomed(PAN_AMOUNT);
      break;
    case DOWN:
      offset.second += zoomed(PAN_AMOUNT);
      break;
    case LEFT:
      offset.first -= zoomed(PAN_AMOUNT);
      break;
    case RIGHT:
      offset.first += zoomed(PAN_AMOUNT);
      break;
  }
}

void enforcePanLimits() {
  if (offset.first < 0)
    offset.first = 0;
  else {
    const auto maxXOffset =
        static_cast<int>(map.width()) - zoomed(renderer.width());
    if (offset.first > maxXOffset) offset.first = maxXOffset;
  }
  if (offset.second < 0)
    offset.second = 0;
  else {
    const auto maxYOffset =
        static_cast<int>(map.height()) - zoomed(renderer.height());
    if (offset.second > maxYOffset) offset.second = maxYOffset;
  }
}

void zoomIn() {
  auto oldWidth = zoomed(renderer.width());
  auto oldHeight = zoomed(renderer.height());
  ++zoomLevel;
  auto newWidth = zoomed(renderer.width());
  auto newHeight = zoomed(renderer.height());
  offset.first += (oldWidth - newWidth) / 2;
  offset.second += (oldHeight - newHeight) / 2;
}

void zoomOut() {
  auto oldWidth = zoomed(renderer.width());
  auto oldHeight = zoomed(renderer.height());
  --zoomLevel;
  auto newWidth = zoomed(renderer.width());
  auto newHeight = zoomed(renderer.height());
  offset.first -= (newWidth - oldWidth) / 2;
  offset.second -= (newHeight - oldHeight) / 2;
}

int zoomed(int value) {
  if (zoomLevel > 0)
    return value >> zoomLevel;
  else if (zoomLevel < 0)
    return value << -zoomLevel;
  return value;
}

int unzoomed(int value) {
  if (zoomLevel < 0)
    return value >> -zoomLevel;
  else if (zoomLevel > 0)
    return value << zoomLevel;
  return value;
}

double zoomed(double value) {
  if (zoomLevel > 0) {
    for (auto i = 0; i != zoomLevel; ++i) value /= 2.0;
    return value;
  } else if (zoomLevel < 0) {
    for (auto i = 0; i != -zoomLevel; ++i) value *= 2.0;
    return value;
  }
  return value;
}

double unzoomed(double value) {
  if (zoomLevel > 0) {
    for (auto i = 0; i != zoomLevel; ++i) value *= 2.0;
    return value;
  } else if (zoomLevel < 0) {
    for (auto i = 0; i != -zoomLevel; ++i) value /= 2.0;
    return value;
  }
  return value;
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
  image.draw(scaledDrawRect + screenPoint);
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
}
