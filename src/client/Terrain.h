// (C) 2016 Tim Gurto

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Texture.h"
#include "../Rect.h"

class Terrain{
    char _index;
    Texture _image;
    bool _isTraversable;

public:
    Terrain(char index, const std::string &imageFile = "", bool isTraversable = true);

    bool operator<(const Terrain &rhs) const;

    void draw(const Rect &loc, const Rect &srcRect) const;
    void draw(int x, int y) const;

    void setQuarterAlpha() const;
    void setHalfAlpha() const;
};

#endif
