// (C) 2015 Tim Gurto

#ifndef PICTURE_H
#define PICTURE_H

#include "Element.h"

// A picture, copied from a Texture.
class Picture : public Element{
    Texture _srcTexture; // Can't use _texture directly, as the Picture may have child Elements.

    virtual void refresh();

public:
    Picture(const SDL_Rect &rect, const Texture &srcTexture);
};

#endif
