#include "Renderer.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cassert>

#include "../Args.h"
#include "../WorkerThread.h"
#include "../types.h"

extern Args cmdLineArgs;
extern Renderer renderer;

size_t Renderer::_count = 0;

#ifdef TESTING
extern WorkerThread SDLThread;
#define SDL_THREAD_BEGIN(...) SDLThread.callBlocking([__VA_ARGS__](){
#define SDL_THREAD_END \
  });
#else
#define SDL_THREAD_BEGIN(...)
#define SDL_THREAD_END
#endif

Renderer::Renderer() : _renderer(nullptr), _window(nullptr), _valid(false) {
  if (_count == 0) {
    // First renderer constructed; initialize SDL
    SDL_THREAD_BEGIN()
    auto ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (ret < 0) return;

    ret = TTF_Init();
    if (ret < 0) assert(false);

    ret = IMG_Init(IMG_INIT_PNG);
    SDL_THREAD_END
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

  SDL_THREAD_BEGIN(this, fullScreen, screenX, screenY, screenW, screenH)
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
  SDL_THREAD_END

  _valid = true;
}

Renderer::~Renderer() {
  SDL_THREAD_BEGIN(this)
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
  SDL_THREAD_END
}

SDL_Texture *Renderer::createTextureFromSurface(SDL_Surface *surface) const {
  if (!_renderer) return nullptr;

  SDL_Texture *texture;
  SDL_THREAD_BEGIN(&texture, surface, this)
  texture = SDL_CreateTextureFromSurface(_renderer, surface);
  SDL_THREAD_END
  return texture;
}

SDL_Texture *Renderer::createTargetableTexture(px_t width, px_t height) const {
  if (!_renderer) return nullptr;

  SDL_Texture *texture;
  SDL_THREAD_BEGIN(&texture, width, height, this)
  texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_TARGET, width, height);
  SDL_THREAD_END
  return texture;
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect) {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this, srcTex, &dstRect)
  SDL_RenderCopy(_renderer, srcTex, 0, &rectToSDL(dstRect));
  SDL_THREAD_END
}

void Renderer::drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect,
                           const ScreenRect &srcRect) {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this, srcTex, &dstRect, &srcRect)
  SDL_RenderCopy(_renderer, srcTex, &rectToSDL(srcRect), &rectToSDL(dstRect));
  SDL_THREAD_END
}

void Renderer::setDrawColor(const Color &color) {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this, color)
  SDL_SetRenderDrawColor(_renderer, color.r(), color.g(), color.b(), 0xff);
  SDL_THREAD_END
}

void Renderer::clear() {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_RenderClear(_renderer);
  SDL_THREAD_END
}

void Renderer::present() {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_RenderPresent(_renderer);
  SDL_THREAD_END
}

void Renderer::drawRect(const ScreenRect &dstRect) {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this, &dstRect)
  SDL_RenderDrawRect(_renderer, &rectToSDL(dstRect));
  SDL_THREAD_END
}

void Renderer::fillRect(const ScreenRect &dstRect) {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this, &dstRect)
  SDL_RenderFillRect(_renderer, &rectToSDL(dstRect));
  SDL_THREAD_END
}

void Renderer::fill() {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_RenderFillRect(_renderer, nullptr);
  SDL_THREAD_END
}

void Renderer::fillWithTransparency() {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0x00);
  SDL_RenderFillRect(_renderer, nullptr);
  SDL_THREAD_END
}

void Renderer::setRenderTarget() const {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_SetRenderTarget(_renderer, 0);
  SDL_THREAD_END
}

void Renderer::updateSize() {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_GetRendererOutputSize(_renderer, &_w, &_h);
  SDL_THREAD_END
}

void Renderer::pushRenderTarget(Texture &target) {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this, &target)
  SDL_Texture *currentTarget = SDL_GetRenderTarget(_renderer);
  _renderTargetsStack.push(currentTarget);
  SDL_SetRenderTarget(_renderer, target.raw());
  SDL_THREAD_END
}

void Renderer::popRenderTarget() {
  if (!_renderer) return;

  SDL_THREAD_BEGIN(this)
  SDL_SetRenderTarget(_renderer, _renderTargetsStack.top());
  _renderTargetsStack.pop();
  SDL_THREAD_END
}

SDL_Rect Renderer::rectToSDL(const ScreenRect &rect) {
  SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
  return r;
}

Color Renderer::getPixel(px_t x, px_t y) const {
  if (!_renderer) return {};

  Uint32 pixel;

  SDL_THREAD_BEGIN(this, &pixel, x, y)
  px_t logicalW, logicalH;

  SDL_RenderGetLogicalSize(_renderer, &logicalW, &logicalH);

  double scaleW = 1.0 * _w / logicalW, scaleH = 1.0 * _h / logicalH;
  auto scaledX = toInt(x * scaleW);
  auto scaledY = toInt(y * scaleH);

  SDL_Surface *windowSurface = SDL_GetWindowSurface(_window);

  const SDL_Rect rect = {scaledX, scaledY, 1, 1};
  auto pitch = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  const auto pixelFormat = SDL_PIXELFORMAT_ARGB8888;
#else
  const auto pixelFormat = SDL_PIXELFORMAT_ABGR8888;
#endif

  SDL_RenderReadPixels(_renderer, &rect, pixelFormat, &pixel, pitch);
  SDL_THREAD_END

  return pixel;
}
