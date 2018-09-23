#ifndef SURFACE_H
#define SURFACE_H

// Wrapper for SDL_Surface.
class Surface {
  std::shared_ptr<SDL_Surface> _raw;

  std::string _description;  // For easier debugging of leaks.

 public:
  Surface() {}
  Surface(const std::string &filename, const Color &colorKey = Color::TODO);
  Surface(TTF_Font *font, const std::string &text,
          const Color &color = Color::TODO);

  bool operator!() const;
  operator bool() const;

  Uint32 getPixel(px_t x, px_t y) const;
  void setPixel(px_t x, px_t y, Uint32 color);
  void swapColors(Uint32 fromColor, Uint32 toColor);

  const std::string &description() const { return _description; }

  SDL_Texture *toTexture() const;
};

#endif
