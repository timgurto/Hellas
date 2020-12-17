#include "Texture.h"

#include <cassert>

#include "../Color.h"
#include "../WorkerThread.h"
#include "Renderer.h"
#include "Surface.h"

extern Renderer renderer;

#ifdef TESTING
extern WorkerThread SDLThread;
#define SDL_THREAD_BEGIN(...) SDLThread.callBlocking([__VA_ARGS__](){
#define SDL_THREAD_END \
  });
#else
#define SDL_THREAD_BEGIN(...)
#define SDL_THREAD_END
#endif

void Texture::freeSurfaceInSDLThread(SDL_Texture *texture) {
  SDL_THREAD_BEGIN(texture)
  SDL_DestroyTexture(texture);
  SDL_THREAD_END
}

Texture::Texture() {}

Texture::Texture(px_t width, px_t height)
    : _w(width), _h(height), _validTarget(true) {
  assert(renderer);

  _raw = {renderer.createTargetableTexture(width, height),
          freeSurfaceInSDLThread};
  if (_raw) _validTarget = false;
}

Texture::Texture(const std::string &filename, const Color &colorKey) {
  if (filename.empty()) return;
  assert(renderer);

  _surface = {filename, colorKey};
  createFromSurface();

  if (!*this) *this = placeholder();
}

Texture::Texture(const Surface &surface) {
  _surface = surface;
  createFromSurface();
}

Texture::Texture(TTF_Font *font, const std::string &text, const Color &color) {
  assert(renderer);

  if (!font) return;

  _surface = {font, text, color};
  createFromSurface();
}

void Texture::createFromSurface() {
  if (!_surface) return;

  _raw = {_surface.toTexture(), freeSurfaceInSDLThread};

  SDL_THREAD_BEGIN(this)
  bool isValid;
  isValid = SDL_QueryTexture(_raw.get(), nullptr, nullptr, &_w, &_h) == 0;
  if (!isValid) _raw = {};
  SDL_THREAD_END
}

Texture::Texture(const Texture &rhs)
    : _raw(rhs._raw),
      _w(rhs._w),
      _h(rhs._h),
      _validTarget(rhs._validTarget),
      _surface(rhs._surface) {}

Texture &Texture::operator=(const Texture &rhs) {
  if (this == &rhs) return *this;
  if (_raw == rhs._raw) return *this;

  _raw = rhs._raw;
  _w = rhs._w;
  _h = rhs._h;
  _validTarget = rhs._validTarget;
  _surface = rhs._surface;

  return *this;
}

void Texture::setBlend(SDL_BlendMode mode) const {
  SDL_THREAD_BEGIN(this, mode)
  SDL_SetTextureBlendMode(_raw.get(), mode);
  SDL_THREAD_END
}

void Texture::setAlpha(Uint8 alpha) const {
  SDL_THREAD_BEGIN(this, alpha)
  SDL_SetTextureAlphaMod(_raw.get(), alpha);
  SDL_THREAD_END
}

void Texture::setColourMod(const Color &colour) {
  SDL_THREAD_BEGIN(this, &colour)
  SDL_SetTextureColorMod(_raw.get(), colour.r(), colour.g(), colour.b());
  SDL_THREAD_END
}

void Texture::rotateClockwise(const ScreenPoint &centre) {
  SDL_THREAD_BEGIN(this, &centre)
  auto centreSDL = SDL_Point{centre.x, centre.y};
  SDL_RenderCopyEx(renderer.raw(), _raw.get(), nullptr, nullptr, 90.0,
                   &centreSDL, SDL_FLIP_NONE);
  SDL_THREAD_END
}

void Texture::draw(px_t x, px_t y) const { draw({x, y, _w, _h}); }

void Texture::draw(const ScreenPoint &location) const {
  draw(toInt(location.x), toInt(location.y));
}

void Texture::draw(const ScreenRect &location) const {
  renderer.drawTexture(_raw.get(), location);
}

void Texture::draw(const ScreenRect &location,
                   const ScreenRect &srcRect) const {
  renderer.drawTexture(_raw.get(), location, srcRect);
}

void Texture::dontDrawPlaceholderIfInvalid() {
  if (_raw == placeholder()._raw) _raw.reset();
}

Color Texture::getPixel(px_t x, px_t y) const {
  if (!_surface) return 0;
  return _surface.getPixel(x, y);
}

void Texture::setRenderTarget() const {
  if (!_validTarget) return;

  SDL_THREAD_BEGIN(this)
  SDL_SetRenderTarget(renderer._renderer, _raw.get());
  SDL_THREAD_END
}

Texture &Texture::placeholder() {
  static auto placeholder = Texture{};
  if (!placeholder) {
    placeholder = {30, 30};
    renderer.pushRenderTarget(placeholder);
    renderer.setDrawColor(Color::BLUE_HELL);
    renderer.fill();
    renderer.setDrawColor(Color::SPRITE_OUTLINE);
    renderer.drawRect({0, 0, 30, 30});
    renderer.popRenderTarget();
  }
  return placeholder;
}
