#include <SDL.h>

#include "../../../src/XmlReader.h"

#include "Map.h"
#include "main.h"

auto loop = true;
SDL_Window *window{nullptr};
SDL_Renderer *renderer{nullptr};
auto map = Map{};
auto zoomLevel = 1;
std::pair<int, int> offset = {0, 0};
int winW{0}, winH{0};

#undef main
int main(int argc, char *argv[]) {
  initialiseSDL();

  map = {"../../Data/map.xml"};

  offset.first = (map.width() - winW) / 2;
  offset.second = (map.height() - winH) / 2;

  auto time = SDL_GetTicks();
  while (loop) {
    auto newTime = SDL_GetTicks();
    auto timeElapsed = newTime - time;
    time = newTime;

    handleInput(timeElapsed);
    render();
  }

  finaliseSDL();
  return 0;
}

void initialiseSDL() {
  SDL_Init(SDL_INIT_VIDEO);

  window =
      SDL_CreateWindow("Hellas Editor", 100, 100, 800, 600, SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_GetRendererOutputSize(renderer, &winW, &winH);
}

void finaliseSDL() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
}

void handleInput(unsigned timeElapsed) {
  auto e = SDL_Event{};
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        loop = false;
        break;

      case SDL_WINDOWEVENT: {
        auto eventType = e.window.event;
        SDL_GetRendererOutputSize(renderer, &winW, &winH);
      } break;

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

  const auto blueHell = Color{24, 82, 161};
  SDL_SetRenderDrawColor(renderer, blueHell.r(), blueHell.g(), blueHell.b(),
                         255);
  auto result = SDL_RenderClear(renderer);

  auto src = SDL_Rect{offset.first, offset.second, zoomed(winW), zoomed(winH)};
  auto dst = SDL_Rect{0, 0, winW, winH};
  result = SDL_RenderCopy(renderer, map.wholeMap(), &src, &dst);
  auto error = SDL_GetError();

  SDL_RenderPresent(renderer);
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
    const auto maxXOffset = static_cast<int>(map.width()) - zoomed(winW);
    if (offset.first > maxXOffset) offset.first = maxXOffset;
  }
  if (offset.second < 0)
    offset.second = 0;
  else {
    const auto maxYOffset = static_cast<int>(map.height()) - zoomed(winH);
    if (offset.second > maxYOffset) offset.second = maxYOffset;
  }
}

void zoomIn() {
  auto oldWidth = zoomed(winW);
  auto oldHeight = zoomed(winH);
  ++zoomLevel;
  auto newWidth = zoomed(winW);
  auto newHeight = zoomed(winH);
  offset.first += (oldWidth - newWidth) / 2;
  offset.second += (oldHeight - newHeight) / 2;
}

void zoomOut() {
  auto oldWidth = zoomed(winW);
  auto oldHeight = zoomed(winH);
  --zoomLevel;
  auto newWidth = zoomed(winW);
  auto newHeight = zoomed(winH);
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
