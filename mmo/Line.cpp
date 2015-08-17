// (C) 2015 Tim Gurto

#include "Line.h"
#include "Renderer.h"

extern Renderer renderer;

Line::Line(int x, int y, int length, Orientation orientation):
Element(makeRect(x, y, 2, 2)),
_orientation(orientation){
    if (_orientation == HORIZONTAL)
        width(length);
    else
        height(length);
}

void Line::refresh(){
    renderer.pushRenderTarget(_texture);

    const SDL_Rect darkRect = _orientation == HORIZONTAL ?
                               makeRect(0, 0, rect().w, 1) :
                               makeRect(0, 0, 1, rect().h);
    renderer.setDrawColor(SHADOW_DARK);
    renderer.fillRect(darkRect);

    const SDL_Rect lightRect = _orientation == HORIZONTAL ?
                                makeRect(0, 1, rect().w, 1) :
                                makeRect(1, 0, 1, rect().h);
    renderer.setDrawColor(SHADOW_LIGHT);
    renderer.fillRect(lightRect);

    drawChildren();

    renderer.popRenderTarget();
}
