// (C) 2016 Tim Gurto

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Texture.h"
#include "../Rect.h"

class Terrain{
    std::vector<Texture> _images;
    bool _isTraversable;
    size_t _frames;
    size_t _frame;
    Uint32 _frameTime;
    Uint32 _frameTimer;

public:
    Terrain(const std::string &imageFile = "", bool isTraversable = true, size_t frames = 1,
            Uint32 frameTime = 0);

    void draw(const Rect &loc, const Rect &srcRect) const;
    void draw(int x, int y) const;

    void setQuarterAlpha() const;
    void setHalfAlpha() const;

    void advanceTime(Uint32 timeElapsed);
};

#endif
