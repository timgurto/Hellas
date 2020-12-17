#include "Surface.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cassert>
#include <string>

#include "../Color.h"
#include "../WorkerThread.h"
#include "Renderer.h"

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

void Surface::freeSurfaceInSDLThread(SDL_Surface *surface) {
  SDL_THREAD_BEGIN(surface)
  SDL_FreeSurface(surface);
  SDL_THREAD_END
}

Surface::Surface(const std::string &filename, const Color &colorKey) {
  SDL_THREAD_BEGIN(this, &filename, colorKey)
  auto *rawPointer = IMG_Load(filename.c_str());
  _raw = {rawPointer, freeSurfaceInSDLThread};
  if (!_raw) return;

  if (&colorKey != &Color::NO_KEY)
    SDL_SetColorKey(_raw.get(), SDL_TRUE, colorKey);
  SDL_THREAD_END

  if (isDebug()) _description = filename;
}

Surface::Surface(TTF_Font *font, const std::string &text, const Color &color) {
  SDL_THREAD_BEGIN(this, &text, color, font)
  auto *rawPointer = TTF_RenderText_Blended(font, text.c_str(), color);
  _raw = {rawPointer, freeSurfaceInSDLThread};
  SDL_THREAD_END

  if (isDebug()) _description = "Text: " + text;
}

SDL_Texture *Surface::toTexture() const {
  return renderer.createTextureFromSurface(_raw.get());
}

bool Surface::operator!() const { return !_raw.operator bool(); }

Surface::operator bool() const { return _raw.operator bool(); }

Uint32 Surface::getPixel(px_t x, px_t y) const {
  int bytesPerPixel = _raw->format->BytesPerPixel;
  Uint8 *p = (Uint8 *)_raw->pixels + y * _raw->pitch + x * bytesPerPixel;

  switch (bytesPerPixel) {
    case 1:
      return *p;
    case 2:
      return *reinterpret_cast<Uint16 *>(p);
    case 4:
      return *reinterpret_cast<Uint32 *>(p);
    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;

    default:
      assert(false);
      return 0;
  }
}

void Surface::setPixel(px_t x, px_t y, Uint32 color) {
  int bytesPerPixel = _raw->format->BytesPerPixel;
  Uint8 *p = (Uint8 *)_raw->pixels + y * _raw->pitch + x * bytesPerPixel;
  switch (bytesPerPixel) {
    case 1:
      *p = color;
      break;

    case 2:
      *reinterpret_cast<Uint16 *>(p) = color;
      break;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (color >> 16) & 0xff;
        p[1] = (color >> 8) & 0xff;
        p[2] = color & 0xff;
      } else {
        p[0] = color & 0xff;
        p[1] = (color >> 8) & 0xff;
        p[2] = (color >> 16) & 0xff;
      }
      break;

    case 4:
      *reinterpret_cast<Uint32 *>(p) = color;
      break;
  }
}

bool Surface::isPixelVisible(size_t x, size_t y, Uint32 colorKey) const {
  const auto format = _raw->format;
  const auto pixel = getPixel(x, y);
  switch (format->BytesPerPixel) {
    case 3:
      return pixel != colorKey;
    case 4:
      return (pixel & format->Amask) > 0;
  }
  return false;
}

void Surface::swapAllVisibleColors(Uint32 toColor) {
  if (_raw == nullptr) return;
  auto colorKey = Uint32{};
  SDL_GetColorKey(_raw.get(), &colorKey);

  for (size_t x = 0; x != _raw->w; ++x)
    for (size_t y = 0; y != _raw->h; ++y)
      if (isPixelVisible(x, y, colorKey)) setPixel(x, y, toColor);
}
