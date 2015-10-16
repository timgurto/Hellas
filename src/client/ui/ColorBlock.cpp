// (C) 2015 Tim Gurto

#include "ColorBlock.h"
#include "../Renderer.h"

extern Renderer renderer;

ColorBlock::ColorBlock(const Rect &rect, const Color &color):
Element(rect),
_color(color){}

void ColorBlock::refresh(){
    renderer.pushRenderTarget(_texture);

    renderer.setDrawColor(_color);
    renderer.fillRect(Rect(0, 0, rect().w, rect().h));

    drawChildren();

    renderer.popRenderTarget();
}
