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

    static SDL_Window *_window; // TODO: Move to a new 'Renderer' class
    static bool _initialized;
    static void initRenderer(); // Initializes window and renderer on first c'tor
    static void destroyRenderer(); // Frees window/renderer on last d'tor
    static std::map<SDL_Texture *, size_t> _refs;
    void addRef(); // Increment reference counter, and initialize window and renderer on first run
    void removeRef(); // Decrement reference counter, and free memory/uninitialize if needed
    static int _numTextures; // The total number of texture *references* in use

    bool _programEndMarker;
    Texture(bool programEndMarker) : _programEndMarker(true) {}
    static Texture _programEndMarkerTexture;

public:
    static SDL_Renderer *renderer; // TODO: Move to a new 'Renderer' class

    Texture();
    Texture(const std::string &filename, const Color &colorKey = Color::NO_KEY);
    Texture(TTF_Font *font, const std::string &text, const Color &color = Color::WHITE);
    ~Texture();

    Texture(const Texture &rhs);
    Texture &operator=(const Texture &rhs);

    bool operator!() const;
    operator bool() const;

    int width() const;
    int height() const;

    void draw(int x, int y) const;
    void draw(const Point &location) const;
    void draw(const SDL_Rect &location) const;

    static int numTextures();

    static void allowConstruction();
    static void forbidDestruction();
};

#endif
