#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL.h>
#include <SDL_ttf.h>

#include <map>
#include <memory>
#include <string>

#include "../Color.h"
#include "../Point.h"
#include "Surface.h"

// A wrapper class for SDL_Texture, which also provides related functionality
class Texture {
  std::shared_ptr<SDL_Texture> _raw;
  Surface _surface;  // If created from a surface
  px_t _w{0}, _h{0};
  bool _validTarget{false};

 public:
  Texture();
  Texture(px_t width,
          px_t height);  // Create a blank texture, which can be rendered to
  Texture(const std::string &filename, const Color &colorKey = Color::NO_KEY);
  Texture(const Surface &surface);
  Texture(TTF_Font *font, const std::string &text,
          const Color &color = Color::WINDOW_FONT);
  void createFromSurface();  // Uses _surface

  static void freeSurfaceInSDLThread(SDL_Texture *texture);

  Texture(const Texture &rhs);
  Texture &operator=(const Texture &rhs);

  bool operator!() const { return _raw == nullptr; }
  operator bool() const { return _raw != nullptr; }

  SDL_Texture *raw() { return _raw.get(); }
  px_t width() const { return _w; }
  px_t height() const { return _h; }

  // These functions are const, making blendmode and alpha de-facto mutable
  void setBlend(SDL_BlendMode mode = SDL_BLENDMODE_BLEND) const;
  void setAlpha(Uint8 alpha = 0xff) const;
  void setColourMod(const Color &colour);

  void rotateClockwise(const ScreenPoint &centre);

  void draw(px_t x = 0, px_t y = 0) const;
  void draw(const ScreenPoint &location) const;
  void draw(const ScreenRect &location) const;
  void draw(const ScreenRect &location, const ScreenRect &srcRect) const;

  void dontDrawPlaceholderIfInvalid();

  Color getPixel(px_t x, px_t y) const;

  /*
  Render to this Texture instead of the renderer.
  Texture must have been created with Texture(width, height), otherwise this
  function will have no effect.
  */
  void setRenderTarget() const;

 private:
  static Texture &placeholder();
};

#endif
