#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <map>
#include <string>

#include "Color.h"
#include "Point.h"

// A wrapper class for SDL_Texture, which also provides related functionality
class Texture{
    SDL_Texture *_raw;
    int _w, _h;

    static std::map<SDL_Texture *, size_t> _refs;
    void addRef(); // Increment reference counter, and initialize window and renderer on first run
    void removeRef(); // Decrement reference counter, and free memory/uninitialize if needed
    static int _numTextures; // The total number of texture *references* in use

    bool _programEndMarker;
    Texture(bool programEndMarker) : _programEndMarker(true) {}
    static Texture _programEndMarkerTexture;

public:
    Texture();
    Texture(const std::string &filename, const Color &colorKey = Color::NO_KEY);
    Texture(TTF_Font *font, const std::string &text, const Color &color = Color::WHITE);
    ~Texture();

    Texture(const Texture &rhs);
    Texture &operator=(const Texture &rhs);

    inline bool operator!() const { return _raw == 0; }
    inline operator bool() const { return _raw != 0; }

    inline int width() const { return _w; }
    inline int height() const { return _h; }

    void draw(int x, int y) const;
    void draw(const Point &location) const;
    void draw(const SDL_Rect &location) const;

    inline static int numTextures() { return _numTextures; }
};

#endif
