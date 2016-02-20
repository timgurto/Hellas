// (C) 2016 Tim Gurto

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Texture.h"
#include "../Rect.h"

class Terrain{
    Texture _image;
    bool _isTraversable;

public:
    Terrain(const std::string &imageFile = "", bool isTraversable = true);

    void draw(const Rect &loc, const Rect &srcRect) const;
    void draw(int x, int y) const;

    void setQuarterAlpha() const;
    void setHalfAlpha() const;
};

#endif
