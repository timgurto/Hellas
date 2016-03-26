// (C) 2015-2016 Tim Gurto

#include "Picture.h"
#include "../Renderer.h"

extern Renderer renderer;

Picture::Picture(const Rect &rect, const Texture &srcTexture):
Element(rect),
_srcTexture(srcTexture){}

void Picture::refresh(){
    _srcTexture.draw(Rect(0, 0, rect().w, rect().h));
}

void Picture::changeTexture(const Texture &srcTexture){
    _srcTexture = srcTexture;
    markChanged();
}
