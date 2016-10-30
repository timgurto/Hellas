#ifndef PICTURE_H
#define PICTURE_H

#include "Element.h"

// A picture, copied from a Texture.
class Picture : public Element{
    Texture _srcTexture; // Can't use _texture directly, as the Picture may have child Elements.

    virtual void refresh() override;

public:
    Picture(const Rect &rect, const Texture &srcTexture);
    Picture(px_t x, px_t y, const Texture &srcTexture); // x and y only; get w/h from texture

    void changeTexture(const Texture &srcTexture = Texture());
};

#endif
