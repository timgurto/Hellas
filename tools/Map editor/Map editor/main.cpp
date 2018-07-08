#include <SDL.h>
#include <windows.h>
#include <algorithm>

#include "../../../src/Args.h"
#include "../../../src/XmlReader.h"
#include "../../../src/client/Renderer.h"
#include "../../../src/client/ui/Label.h"

#include "Map.h"
#include "Terrain.h"
#include "main.h"

auto cmdLineArgs = Args{};   // MUST be defined before renderer
auto renderer = Renderer{};  // MUST be defined after cmdLineArgs

auto loop = true;
auto map = Map{};
auto zoomLevel = 1;
std::pair<int, int> offset = {0, 0};
auto mouse = ScreenPoint{};

auto terrain = TerrainType::Container{};

#undef main
int main(int argc, char *argv[]) {
  renderer.init();
  SDL_RenderSetLogicalSize(renderer.raw(), renderer.width(), renderer.height());

  auto files = findDataFiles("../../Data");
  for (const auto &file : files) {
    TerrainType::load(terrain, file);
  }

  map = {"../../Data/map.xml"};
  offset.first = (map.width() - renderer.width()) / 2;
  offset.second = (map.height() - renderer.height()) / 2;

  Element::initialize();
  Element::font(TTF_OpenFont("micross.ttf", 12));

  auto time = SDL_GetTicks();
  while (loop) {
    auto newTime = SDL_GetTicks();
    auto timeElapsed = newTime - time;
    time = newTime;

    handleInput(timeElapsed);
    render();
  }

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
          // for (Window *window : _windows) window->forceRefresh();
          // for (Element *element : _ui) element->forceRefresh();
          // Tooltip::forceAllToRedraw();
      } break;

      case SDL_MOUSEMOTION:
        SDL_GetMouseState(&mouse.x, &mouse.y);
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
        } else if (e.wheel.y > 0) {
          zoomIn();
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

  auto src = ScreenRect{offset.first, offset.second, zoomed(renderer.width()),
                        zoomed(renderer.height())};
  auto dst = ScreenRect{0, 0, renderer.width(), renderer.height()};
  map.wholeMap().draw(dst, src);

  auto mapStretched = src.w > map.width() || src.h > map.height();
  if (mapStretched) {
    renderer.setDrawColor(Color::RED);
    for (auto i = 0; i != 10; ++i)
      renderer.drawRect(
          {i, i, renderer.width() - 2 * i, renderer.height() - 2 * i});
  }

  auto cursorLabelRect = ScreenRect{0, renderer.height() - 20, 200, 20};
  auto mapPos = MapPoint{zoomed(1.0 * mouse.x) + offset.first,
                         zoomed(1.0 * mouse.y) + offset.second} *
                16.0;
  Label{cursorLabelRect,
        "Cursor is at (" + toString(mapPos.x) + "," + toString(mapPos.y) + ")"}
      .draw();

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

FilesList findDataFiles(const std::string &searchPath) {
  auto list = FilesList{};

  WIN32_FIND_DATA fd;
  auto path = std::string{searchPath.begin(), searchPath.end()} + "/";
  std::replace(path.begin(), path.end(), '/', '\\');
  std::string filter = path + "*.xml";
  path.c_str();
  HANDLE hFind = FindFirstFile(filter.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName == std::string{"map.xml"}) continue;
      auto file = path + fd.cFileName;
      list.insert(file);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
  }

  return list;
}
