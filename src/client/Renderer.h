#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <stack>

#include "../Color.h"
#include "../Rect.h"
#include "../util.h"
#include "Texture.h"

// Wrapper class for SDL_Renderer and SDL_Window, plus related convenience
// functions.
class Renderer {
  SDL_Renderer *_renderer;
  SDL_Window *_window;
  px_t _w, _h;
  bool _valid;  // Whether everything has been properly initialized
  static size_t _count;

  std::stack<SDL_Texture *> _renderTargetsStack;

  static SDL_Rect rectToSDL(const ScreenRect &rect);

  friend void Texture::setRenderTarget()
      const;  // Needs access to raw SDL_Renderer

 public:
  Renderer();
  ~Renderer();

  /*
  Some construction takes place here, to be called manually, as a Renderer may
  be static. Specifically, this creates the window and renderer objects, and
  requires that cmdLineArgs has been populated.
  */
  void init();

  operator bool() const { return _valid; }

  px_t width() const { return _w; }
  px_t height() const { return _h; }

  SDL_Texture *createTextureFromSurface(SDL_Surface *surface) const;
  SDL_Texture *createTargetableTexture(px_t width, px_t height) const;
  void drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect);
  void drawTexture(SDL_Texture *srcTex, const ScreenRect &dstRect,
                   const ScreenRect &srcRect);

  void setDrawColor(const Color &color = Color::DEFAULT_DRAW);
  void clear();
  void present();

  void setRenderTarget()
      const;  // Render to renderer instead of any Texture that might be set.

  void drawRect(const ScreenRect &dstRect);
  void fillRect(const ScreenRect &dstRect);
  void fill();

  Color getPixel(px_t x, px_t y) const;

  void updateSize();

  void pushRenderTarget(Texture &target);
  void popRenderTarget();
};

#endif
