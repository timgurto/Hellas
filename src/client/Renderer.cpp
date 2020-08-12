#include "Renderer.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cassert>

#include "../Args.h"
#include "../types.h"
#include "WorkerThread.h"

extern Args cmdLineArgs;
extern Renderer renderer;
extern WorkerThread SDLWorker;

size_t Renderer::_count = 0;

Renderer::Renderer() : _renderer(nullptr), _window(nullptr), _valid(false) {
  if (_count == 0) {
    // First renderer constructed; initialize SDL
    SDLWorker.enqueue([&]() {
      auto ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
      if (ret < 0) return;

      ret = TTF_Init();
      if (ret < 0) assert(false);

      ret = IMG_Init(IMG_INIT_PNG);
    });
  }
  ++_count;
}

void Renderer::init() {
  const px_t screenX = cmdLineArgs.contains("left") ? cmdLineArgs.getInt("left")
                                                    : SDL_WINDOWPOS_UNDEFINED;
  const px_t screenY = cmdLineArgs.contains("top") ? cmdLineArgs.getInt("top")
                                                   : SDL_WINDOWPOS_UNDEFINED;
  const px_t screenW =
      cmdLineArgs.contains("width") ? cmdLineArgs.getInt("width") : 1280;
  const px_t screenH =
      cmdLineArgs.contains("height") ? cmdLineArgs.getInt("height") : 720;

  bool fullScreen = cmdLineArgs.contains("fullscreen");

  SDLWorker.enqueue([&]() {
    _window = SDL_CreateWindow(
        "Hellas (Open Beta)", screenX, screenY, screenW, screenH,
        SDL_WINDOW_SHOWN |
            (fullScreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
  });
  SDLWorker.waitUntilDone();
  if (!_window) return;

  int rendererFlags = SDL_RENDERER_ACCELERATED;
  if (!fullScreen || cmdLineArgs.contains("vsync"))
    rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

  SDLWorker.enqueue(
      [&]() { _renderer = SDL_CreateRenderer(_window, -1, rendererFlags); });
  SDLWorker.waitUntilDone();
  if (!_renderer) return;

  SDLWorker.enqueue([&]() {
    SDL_GetRendererOutputSize(_renderer, &_w, &_h);
    SDL_RenderSetLogicalSize(_renderer, 640, 360);
  });
  SDLWorker.waitUntilDone();

  _valid = true;
}

Renderer::~Renderer() {
  SDLWorker.enqueue([&]() {
    if (_renderer) {
      auto temp = _renderer;
      _renderer = nullptr;
      SDL_DestroyRenderer(temp);
    }
    if (!_window) {
      SDL_DestroyWindow(_window);
      _window = nullptr;
    }

    --_count;
    if (_count == 0) {
      TTF_Quit();
      SDL_Quit();
    }
  });
}

SDL_Texture *Renderer::createTextureFromSurface(SDL_Surface *surface) const {
  if (!_renderer) return nullptr;

  SDL_Texture *texture;
  SDLWorker.enqueue(
      [&]() { texture = SDL_CreateTextureFromSurface(_renderer, surface); });
  SDLWorker.waitUntilDone();
  return texture;
}

SDL_Texture *Renderer::createTargetableTexture(px_t width, px_t height) const {
  if (!_renderer) return nullptr;

  SDL_Texture *texture;
  SDLWorker.enqueue([&]() {
    texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_TARGET, width, height);
  });
  SDLWorker.waitUntilDone();
  return texture;
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect) {
  if (!_renderer) return;

  SDLWorker.enqueue(
      [&]() { SDL_RenderCopy(_renderer, srcTex, 0, &rectToSDL(dstRect)); });
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect,
                           const ScreenRect &srcRect) {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() {
    SDL_RenderCopy(_renderer, srcTex, &rectToSDL(srcRect), &rectToSDL(dstRect));
  });
}

void Renderer::setDrawColor(const Color &color) {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() {
    SDL_SetRenderDrawColor(_renderer, color.r(), color.g(), color.b(), 0xff);
  });
}

void Renderer::clear() {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() { SDL_RenderClear(_renderer); });
}

void Renderer::present() {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() { SDL_RenderPresent(_renderer); });
}

void Renderer::drawRect(const ScreenRect &dstRect) {
  if (!_renderer) return;

  SDLWorker.enqueue(
      [&]() { SDL_RenderDrawRect(_renderer, &rectToSDL(dstRect)); });
}

void Renderer::fillRect(const ScreenRect &dstRect) {
  if (!_renderer) return;

  SDLWorker.enqueue(
      [&]() { SDL_RenderFillRect(_renderer, &rectToSDL(dstRect)); });
}

void Renderer::fill() {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() { SDL_RenderFillRect(_renderer, nullptr); });
}

void Renderer::fillWithTransparency() {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() {
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0x00);
    SDL_RenderFillRect(_renderer, nullptr);
  });
}

void Renderer::setRenderTarget() const {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() { SDL_SetRenderTarget(_renderer, 0); });
}

void Renderer::updateSize() {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() { SDL_GetRendererOutputSize(_renderer, &_w, &_h); });
}

void Renderer::pushRenderTarget(Texture &target) {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() {
    SDL_Texture *currentTarget = SDL_GetRenderTarget(_renderer);
    _renderTargetsStack.push(currentTarget);
    SDL_SetRenderTarget(_renderer, target.raw());
  });
}

void Renderer::popRenderTarget() {
  if (!_renderer) return;

  SDLWorker.enqueue([&]() {
    SDL_SetRenderTarget(_renderer, _renderTargetsStack.top());
    _renderTargetsStack.pop();
  });
}

SDL_Rect Renderer::rectToSDL(const ScreenRect &rect) {
  SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
  return r;
}

Color Renderer::getPixel(px_t x, px_t y) const {
  if (!_renderer) return {};
  px_t logicalW, logicalH;

  Uint32 pixel;

  SDLWorker.enqueue([&]() {
    SDL_RenderGetLogicalSize(_renderer, &logicalW, &logicalH);

    double scaleW = 1.0 * _w / logicalW, scaleH = 1.0 * _h / logicalH;
    x = toInt(x * scaleW);
    y = toInt(y * scaleH);

    SDL_Surface *windowSurface = SDL_GetWindowSurface(_window);

    const SDL_Rect rect = {x, y, 1, 1};
    auto pitch = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const auto pixelFormat = SDL_PIXELFORMAT_ARGB8888;
#else
    const auto pixelFormat = SDL_PIXELFORMAT_ABGR8888;
#endif

    SDL_RenderReadPixels(_renderer, &rect, pixelFormat, &pixel, pitch);
  });
  SDLWorker.waitUntilDone();

  return pixel;
}
