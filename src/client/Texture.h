// (C) 2015-2016 Tim Gurto

#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <map>
#include <string>

#include "../Color.h"
#include "../Point.h"

// A wrapper class for SDL_Texture, which also provides related functionality
class Texture{
    SDL_Texture *_raw;
    px_t _w, _h;
    bool _validTarget;

    static std::map<SDL_Texture *, size_t> _refs;
    void addRef(); // Increment reference counter, and initialize window and renderer on first run
    void removeRef(); // Decrement reference counter, and free memory/uninitialize if needed
    static int _numTextures; // The total number of texture *references* in use

    bool _programEndMarker;
    Texture(bool programEndMarker) : _programEndMarker(true) {}
    static Texture _programEndMarkerTexture;

public:
    Texture();
    Texture(px_t width, px_t height); // Create a blank texture, which can be rendered to
    Texture(const std::string &filename, const Color &colorKey = Color::NO_KEY);
    Texture(TTF_Font *font, const std::string &text, const Color &color = Color::MMO_L_GREY);
    ~Texture();

    Texture(const Texture &rhs);
    Texture &operator=(const Texture &rhs);

    bool operator!() const { return _raw == nullptr; }
    operator bool() const { return _raw != nullptr; }

    SDL_Texture *raw() { return _raw; }
    static int numTextures() { return _numTextures; }
    px_t width() const { return _w; }
    px_t height() const { return _h; }

    // These functions are const, making blendmode and alpha de-facto mutable
    void setBlend(SDL_BlendMode mode = SDL_BLENDMODE_BLEND) const;
    void setAlpha(Uint8 alpha = 0xff) const;

    void draw(px_t x = 0, px_t y = 0) const;
    void draw(const Point &location) const;
    void draw(const Rect &location) const;
    void draw(const Rect &location, const Rect &srcRect) const;

    /*
    Render to this Texture instead of the renderer.
    Texture must have been created with Texture(width, height), otherwise this function will have
    no effect.
    */
    void setRenderTarget() const;
};

#endif
