// (C) 2015 Tim Gurto

#include "ShadowBox.h"

extern Renderer renderer;

ShadowBox::ShadowBox(const Rect &rect, bool reversed):
Element(rect),
_reversed(reversed){}

void ShadowBox::refresh(){
    const int
        width = rect().w,
        height = rect().h,
        left = 0,
        right = left + width - 1,
        top = 0,
        bottom = top + height - 1;

    renderer.setDrawColor(_reversed ? SHADOW_DARK : SHADOW_LIGHT);
    renderer.fillRect(Rect(left, top, width - 1, 1));
    renderer.fillRect(Rect(left, top, 1, height - 1));
    renderer.setDrawColor(_reversed ? SHADOW_LIGHT : SHADOW_DARK);
    renderer.fillRect(Rect(right, top + 1, 1, height - 1));
    renderer.fillRect(Rect(left + 1, bottom, width - 1, 1));
}

void ShadowBox::setReversed(bool reversed){
    _reversed = reversed;
    markChanged();
}
