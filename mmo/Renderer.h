// (C) 2015 Tim Gurto

#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>

#include "Color.h"
#include "Texture.h"
#include "util.h"

// Wrapper class for SDL_Renderer and SDL_Window, plus related convenience functions.
class Renderer{
    SDL_Renderer *_renderer;
    SDL_Window *_window;
    int _w, _h;
    bool _valid; // Whether everything has been properly initialized
    static size_t _count;

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

    int width() const { return _w; }
    int height() const { return _h; }

    SDL_Texture *createTextureFromSurface(SDL_Surface *surface) const;
    SDL_Texture *createTargetableTexture(int width, int height) const;
    void drawTexture(SDL_Texture *srcTex, const SDL_Rect &dstRect);
    void drawTexture(SDL_Texture *srcTex, const SDL_Rect &dstRect, const SDL_Rect &srcRect);

    void setDrawColor(const Color &color = Color::BLACK);
    void clear();
    void present();

    void setRenderTarget() const; // Render to renderer instead of any Texture that might be set.

    void drawRect(const SDL_Rect &dstRect);
    void fillRect(const SDL_Rect &dstRect);

    void setScale(float x, float y);

    void updateSize();
};

#endif
