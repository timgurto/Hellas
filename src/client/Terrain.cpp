// (C) 2016 Tim Gurto

#include <cassert>

#include "Client.h"
#include "Terrain.h"
#include "../util.h"

Terrain::Terrain(const std::string &imageFile, bool isTraversable):
_image(std::string("Images/Terrain/") + imageFile + ".png"),
_isTraversable(isTraversable)
{
    if (!isDebug()) {
        _image.setBlend(SDL_BLENDMODE_ADD);
        setQuarterAlpha();
    }
}

void Terrain::draw(const Rect &loc, const Rect &srcRect) const{
    _image.draw(loc, srcRect);
}

void Terrain::draw(int x, int y) const{
    _image.draw(x, y);
}

void Terrain::setHalfAlpha() const{
    _image.setAlpha(0x7f);
}

void Terrain::setQuarterAlpha() const{
    _image.setAlpha(0x3f);
}
