#ifndef TERRAIN_H
#define TERRAIN_H

#include "Texture.h"
#include "../Rect.h"
#include "../types.h"

class ClientTerrain{
    std::vector<Texture> _images;
    size_t _frames;
    size_t _frame;
    ms_t _frameTime;
    ms_t _frameTimer;

public:
    ClientTerrain(const std::string &imageFile = "", size_t frames = 1, ms_t frameTime = 0);

    void draw(const Rect &loc, const Rect &srcRect) const;
    void draw(px_t x, px_t y) const;

    void setQuarterAlpha() const;
    void setHalfAlpha() const;

    void advanceTime(ms_t timeElapsed);
};

#endif
