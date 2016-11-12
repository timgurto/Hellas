#ifndef SURFACE_H
#define SURFACE_H

// Wrapper for SDL_Surface.
class Surface{
    SDL_Surface *_raw;

public:
    Surface(const std::string &filename, const Color &colorKey = Color::NO_KEY);
    Surface(TTF_Font *font, const std::string &text, const Color &color = Color::FONT);
    ~Surface();
    
    bool operator!() const;
    operator bool() const;

    Uint32 getPixel(px_t x, px_t y) const;
    void setPixel(px_t x, px_t y, Uint32 color);

    SDL_Texture *toTexture() const;
};

#endif
