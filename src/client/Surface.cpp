#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cassert>
#include <string>

#include "../Color.h"
#include "Renderer.h"
#include "Surface.h"

extern Renderer renderer;

Surface::Surface(const std::string &filename, const Color &colorKey)
    : _raw(IMG_Load(filename.c_str())) {
  if (_raw == nullptr) return;
  if (&colorKey != &Color::NO_KEY) SDL_SetColorKey(_raw, SDL_TRUE, colorKey);

  if (isDebug()) _description = filename;
}

Surface::Surface(TTF_Font *font, const std::string &text, const Color &color)
    : _raw(TTF_RenderText_Solid(font, text.c_str(), color)) {
  if (isDebug()) _description = "Text: " + text;
}

Surface::~Surface() {
  if (_raw != nullptr) SDL_FreeSurface(_raw);
}

SDL_Texture *Surface::toTexture() const {
  return renderer.createTextureFromSurface(_raw);
}

bool Surface::operator!() const { return _raw == nullptr; }

Surface::operator bool() const { return _raw != nullptr; }

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

void Surface::swapColors(Uint32 fromColor, Uint32 toColor) {
  if (_raw == nullptr) return;
  for (size_t x = 0; x != _raw->w; ++x)
    for (size_t y = 0; y != _raw->h; ++y)
      if (getPixel(x, y) == fromColor) setPixel(x, y, toColor);
}
