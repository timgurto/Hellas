#ifndef SURFACE_H
#define SURFACE_H

#include <SDL.h>
#include <SDL_TTF.h>

#include <memory>
#include <string>

#include "../Color.h"
#include "../types.h"

// Wrapper for SDL_Surface.
class Surface {
  std::shared_ptr<SDL_Surface> _raw;

  std::string _description;  // For easier debugging of leaks.

 public:
  Surface() {}
  Surface(const std::string &filename, const Color &colorKey = Color::NO_KEY);
  Surface(TTF_Font *font, const std::string &text,
          const Color &color = Color::NO_KEY);

  bool operator!() const;
  operator bool() const;

  Uint32 getPixel(px_t x, px_t y) const;
  void setPixel(px_t x, px_t y, Uint32 color);
  void swapAllVisibleColors(Uint32 toColor);
  bool isPixelVisible(size_t x, size_t y, Uint32 colorKey) const;

  const std::string &description() const { return _description; }

  SDL_Texture *toTexture() const;
};

#endif
