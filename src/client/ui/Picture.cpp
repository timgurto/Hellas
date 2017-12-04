#include "Picture.h"
#include "../Renderer.h"

extern Renderer renderer;

Picture::Picture(const ScreenRect &rect, const Texture &srcTexture):
Element(rect),
_srcTexture(srcTexture){}

Picture::Picture(px_t x, px_t y, const Texture &srcTexture):
    Element({ x, y, srcTexture.width(), srcTexture.height() }),
_srcTexture(srcTexture){}

void Picture::refresh(){
    _srcTexture.draw({ 0, 0, rect().w, rect().h });
}

void Picture::changeTexture(const Texture &srcTexture){
    _srcTexture = srcTexture;
    markChanged();
}
