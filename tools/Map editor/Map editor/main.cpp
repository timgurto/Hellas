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
bool shouldRender = false;

#undef main
int main(int argc, char *argv[]) {
  initialiseSDL();

  map = {"../../Data/map.xml"};

  render();

  auto time = SDL_GetTicks();
  while (loop) {
    auto newTime = SDL_GetTicks();
    auto timeElapsed = newTime - time;
    time = newTime;

    handleInput(timeElapsed);
  }

  finaliseSDL();
  return 0;
}

void initialiseSDL() {
  SDL_Init(SDL_INIT_VIDEO);

  window = SDL_CreateWindow("Hellas Editor", 100, 100, 1280, 720,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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

      case SDL_WINDOWEVENT:
        switch (e.window.event)
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
          shouldRender = true;
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
          if (zoomLevel > 1) --zoomLevel;
        } else if (e.wheel.y > 0) {
          ++zoomLevel;
        }
        shouldRender = true;
        break;
    }

    if (shouldRender) render();
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
  int winW, winH;
  SDL_GetRendererOutputSize(renderer, &winW, &winH);

  const auto blueHell = Color{24, 82, 161};
  SDL_SetRenderDrawColor(renderer, blueHell.r(), blueHell.g(), blueHell.b(),
                         255);
  SDL_RenderClear(renderer);

  auto src =
      SDL_Rect{offset.first, offset.second, winW / zoomLevel, winH / zoomLevel};
  auto wholeMap = map.wholeMap();
  auto result = SDL_RenderCopy(renderer, wholeMap, &src, nullptr);
  auto error = SDL_GetError();

  SDL_RenderPresent(renderer);

  shouldRender = false;
}

void pan(Direction dir) {
  const auto PAN_AMOUNT = 200;
  switch (dir) {
    case UP:
      offset.second -= PAN_AMOUNT / zoomLevel;
      break;
    case DOWN:
      offset.second += PAN_AMOUNT / zoomLevel;
      break;
    case LEFT:
      offset.first -= PAN_AMOUNT / zoomLevel;
      break;
    case RIGHT:
      offset.first += PAN_AMOUNT / zoomLevel;
      break;
  }
  shouldRender = true;
}
