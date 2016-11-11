#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <stack>

#include "Texture.h"
#include "../Color.h"
#include "../Rect.h"
#include "../util.h"

// Wrapper class for SDL_Renderer and SDL_Window, plus related convenience functions.
class Renderer{
    SDL_Renderer *_renderer;
    SDL_Window *_window;
    px_t _w, _h;
    bool _valid; // Whether everything has been properly initialized
    static size_t _count;

    std::stack<SDL_Texture *> _renderTargetsStack;

    static SDL_Rect rectToSDL(const Rect &rect);

    friend void Texture::setRenderTarget() const; // Needs access to raw SDL_Renderer

public:
    Renderer();
    ~Renderer();

    /*
    Some construction takes place here, to be called manually, as a Renderer may be static.
    Specifically, this creates the window and renderer objects, and requires that cmdLineArgs has been populated.
    */
    void init();

    operator bool() const { return _valid; }

    px_t width() const { return _w; }
    px_t height() const { return _h; }

    SDL_Texture *createTextureFromSurface(SDL_Surface *surface) const;
    SDL_Texture *createTargetableTexture(px_t width, px_t height) const;
    void drawTexture(SDL_Texture *srcTex, const Rect &dstRect);
    void drawTexture(SDL_Texture *srcTex, const Rect &dstRect, const Rect &srcRect);

    void setDrawColor(const Color &color = Color::DEFAULT_DRAW);
    void clear();
    void present();

    void setRenderTarget() const; // Render to renderer instead of any Texture that might be set.

    void drawRect(const Rect &dstRect);
    void fillRect(const Rect &dstRect);
    void fill();

    void setScale(float x, float y);

    void updateSize();

    void pushRenderTarget(Texture &target);
    void popRenderTarget();
    
    static Uint32 getPixel(SDL_Surface *surface, px_t x, px_t y);
    static void setPixel(SDL_Surface *surface, px_t x, px_t y, Uint32 color);
};

#endif
