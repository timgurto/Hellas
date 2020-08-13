#include "Renderer.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cassert>

#include "../Args.h"
#include "../types.h"
#include "WorkerThread.h"

extern Args cmdLineArgs;
extern Renderer renderer;
extern WorkerThread SDLThread;

size_t Renderer::_count = 0;

Renderer::Renderer() : _renderer(nullptr), _window(nullptr), _valid(false) {
  if (_count == 0) {
    // First renderer constructed; initialize SDL
    SDLThread.enqueue([&]() {
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
  SDLThread.assertIsExecutingThis();
  const px_t screenX = cmdLineArgs.contains("left") ? cmdLineArgs.getInt("left")
                                                    : SDL_WINDOWPOS_UNDEFINED;
  const px_t screenY = cmdLineArgs.contains("top") ? cmdLineArgs.getInt("top")
                                                   : SDL_WINDOWPOS_UNDEFINED;
  const px_t screenW =
      cmdLineArgs.contains("width") ? cmdLineArgs.getInt("width") : 1280;
  const px_t screenH =
      cmdLineArgs.contains("height") ? cmdLineArgs.getInt("height") : 720;

  bool fullScreen = cmdLineArgs.contains("fullscreen");

  _window =
      SDL_CreateWindow("Hellas (Open Beta)", screenX, screenY, screenW, screenH,
                       SDL_WINDOW_SHOWN | (fullScreen ? SDL_WINDOW_FULLSCREEN
                                                      : SDL_WINDOW_RESIZABLE));
  if (!_window) return;

  int rendererFlags = SDL_RENDERER_ACCELERATED;
  if (!fullScreen || cmdLineArgs.contains("vsync"))
    rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

  _renderer = SDL_CreateRenderer(_window, -1, rendererFlags);
  if (!_renderer) return;

  SDL_GetRendererOutputSize(_renderer, &_w, &_h);
  SDL_RenderSetLogicalSize(_renderer, 640, 360);

  _valid = true;
}

Renderer::~Renderer() {
  SDLThread.enqueue([&]() {
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
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return nullptr;

  return SDL_CreateTextureFromSurface(_renderer, surface);
}

SDL_Texture *Renderer::createTargetableTexture(px_t width, px_t height) const {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return nullptr;

  return SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888,
                           SDL_TEXTUREACCESS_TARGET, width, height);
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect) {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderCopy(_renderer, srcTex, 0, &rectToSDL(dstRect));
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect,
                           const ScreenRect &srcRect) {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderCopy(_renderer, srcTex, &rectToSDL(srcRect), &rectToSDL(dstRect));
}

void Renderer::setDrawColor(const Color &color) {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_SetRenderDrawColor(_renderer, color.r(), color.g(), color.b(), 0xff);
}

void Renderer::clear() {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderClear(_renderer);
}

void Renderer::present() {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderPresent(_renderer);
}

void Renderer::drawRect(const ScreenRect &dstRect) {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderDrawRect(_renderer, &rectToSDL(dstRect));
}

void Renderer::fillRect(const ScreenRect &dstRect) {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderFillRect(_renderer, &rectToSDL(dstRect));
}

void Renderer::fill() {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_RenderFillRect(_renderer, nullptr);
}

void Renderer::fillWithTransparency() {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0x00);
  SDL_RenderFillRect(_renderer, nullptr);
}

void Renderer::setRenderTarget() const {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_SetRenderTarget(_renderer, 0);
}

void Renderer::updateSize() {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_GetRendererOutputSize(_renderer, &_w, &_h);
}

void Renderer::pushRenderTarget(Texture &target) {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_Texture *currentTarget = SDL_GetRenderTarget(_renderer);
  _renderTargetsStack.push(currentTarget);
  SDL_SetRenderTarget(_renderer, target.raw());
}

void Renderer::popRenderTarget() {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return;

  SDL_SetRenderTarget(_renderer, _renderTargetsStack.top());
  _renderTargetsStack.pop();
}

SDL_Rect Renderer::rectToSDL(const ScreenRect &rect) {
  SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
  return r;
}

Color Renderer::getPixel(px_t x, px_t y) const {
  SDLThread.assertIsExecutingThis();
  if (!_renderer) return {};
  px_t logicalW, logicalH;

  Uint32 pixel;

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

  return pixel;
}
