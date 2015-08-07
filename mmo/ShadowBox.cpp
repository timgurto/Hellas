// (C) 2015 Tim Gurto

#include "ShadowBox.h"

extern Renderer renderer;

ShadowBox::ShadowBox(const SDL_Rect &rect, bool reversed):
Element(rect),
_reversed(reversed){}

void ShadowBox::refresh(){
    renderer.pushRenderTarget(_texture);

    makeBackgroundTransparent();

    const int
        width = rect().w,
        height = rect().h,
        left = rect().x,
        right = left + width - 1,
        top = rect().y,
        bottom = top + height - 1;

    renderer.setDrawColor(_reversed ? SHADOW_DARK : SHADOW_LIGHT);
    renderer.fillRect(makeRect(left, top, width - 1, 1));
    renderer.fillRect(makeRect(left, top, 1, height - 1));
    renderer.setDrawColor(_reversed ? SHADOW_LIGHT : SHADOW_DARK);
    renderer.fillRect(makeRect(right, top + 1, 1, height - 1));
    renderer.fillRect(makeRect(left + 1, bottom, width - 1, 1));

    drawChildren();

    renderer.popRenderTarget();
}
