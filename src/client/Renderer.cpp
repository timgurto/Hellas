#include "Renderer.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cassert>

#include "../Args.h"
#include "../types.h"

extern Args cmdLineArgs;

size_t Renderer::_count = 0;

Renderer::Renderer() : _renderer(nullptr), _window(nullptr), _valid(false) {
  if (_count == 0) {
    // First renderer constructed; initialize SDL
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (ret < 0) return;
    ret = TTF_Init();
    if (ret < 0) assert(false);
    ret = IMG_Init(IMG_INIT_PNG);
  }
  ++_count;

#ifdef TESTING
  mutex = SDL_CreateMutex();
#endif
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
#ifdef TESTING
  SDL_DestroyMutex(mutex);
#endif

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
}

#ifdef TESTING
#define LOCK_MUTEX SDL_LockMutex(mutex);
#define UNLOCK_MUTEX SDL_UnlockMutex(mutex);
#else
#define LOCK_MUTEX
#define UNLOCK_MUTEX
#endif

SDL_Texture *Renderer::createTextureFromSurface(SDL_Surface *surface) const {
  if (!_renderer) return nullptr;
  LOCK_MUTEX
  auto texture = SDL_CreateTextureFromSurface(_renderer, surface);
  UNLOCK_MUTEX
  return texture;
}

SDL_Texture *Renderer::createTargetableTexture(px_t width, px_t height) const {
  if (!_renderer) return nullptr;
  LOCK_MUTEX
  auto texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_TARGET, width, height);
  UNLOCK_MUTEX
  return texture;
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect) {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderCopy(_renderer, srcTex, 0, &rectToSDL(dstRect));
  UNLOCK_MUTEX
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect,
                           const ScreenRect &srcRect) {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderCopy(_renderer, srcTex, &rectToSDL(srcRect), &rectToSDL(dstRect));
  UNLOCK_MUTEX
}

void Renderer::setDrawColor(const Color &color) {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_SetRenderDrawColor(_renderer, color.r(), color.g(), color.b(), 0xff);
  UNLOCK_MUTEX
}

void Renderer::clear() {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderClear(_renderer);
  UNLOCK_MUTEX
}

void Renderer::present() {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderPresent(_renderer);
  UNLOCK_MUTEX
}

void Renderer::drawRect(const ScreenRect &dstRect) {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderDrawRect(_renderer, &rectToSDL(dstRect));
  UNLOCK_MUTEX
}

void Renderer::fillRect(const ScreenRect &dstRect) {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderFillRect(_renderer, &rectToSDL(dstRect));
  UNLOCK_MUTEX
}

void Renderer::fill() {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_RenderFillRect(_renderer, nullptr);
  UNLOCK_MUTEX
}

void Renderer::fillWithTransparency() {
  LOCK_MUTEX
  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0x00);
  SDL_RenderFillRect(_renderer, nullptr);
  UNLOCK_MUTEX
}

void Renderer::setRenderTarget() const {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_SetRenderTarget(_renderer, 0);
  UNLOCK_MUTEX
}

void Renderer::updateSize() {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_GetRendererOutputSize(_renderer, &_w, &_h);
  UNLOCK_MUTEX
}

void Renderer::pushRenderTarget(Texture &target) {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_Texture *currentTarget = SDL_GetRenderTarget(_renderer);
  _renderTargetsStack.push(currentTarget);
  SDL_SetRenderTarget(_renderer, target.raw());
  UNLOCK_MUTEX
}

void Renderer::popRenderTarget() {
  if (!_renderer) return;
  LOCK_MUTEX
  SDL_SetRenderTarget(_renderer, _renderTargetsStack.top());
  _renderTargetsStack.pop();
  UNLOCK_MUTEX
}

SDL_Rect Renderer::rectToSDL(const ScreenRect &rect) {
  SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
  return r;
}

Color Renderer::getPixel(px_t x, px_t y) const {
  if (!_renderer) return {};
  px_t logicalW, logicalH;
  LOCK_MUTEX
  SDL_RenderGetLogicalSize(_renderer, &logicalW, &logicalH);

  double scaleW = 1.0 * _w / logicalW, scaleH = 1.0 * _h / logicalH;
  x = toInt(x * scaleW);
  y = toInt(y * scaleH);

  SDL_Surface *windowSurface = SDL_GetWindowSurface(_window);

  Uint32 pixel;

  const SDL_Rect rect = {x, y, 1, 1};
  auto pitch = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  const auto pixelFormat = SDL_PIXELFORMAT_ARGB8888;
#else
  const auto pixelFormat = SDL_PIXELFORMAT_ABGR8888;
#endif

  SDL_RenderReadPixels(_renderer, &rect, pixelFormat, &pixel, pitch);
  UNLOCK_MUTEX

  return pixel;
}
