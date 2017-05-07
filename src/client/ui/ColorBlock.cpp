#include "ColorBlock.h"
#include "../Renderer.h"

extern Renderer renderer;

ColorBlock::ColorBlock(const Rect &rect, const Color &color):
Element(rect),
_color(color){}

void ColorBlock::changeColor(const Color &newColor){
    _color = newColor;
    markChanged();
}

void ColorBlock::refresh(){
    renderer.setDrawColor(_color);
    renderer.fillRect(Rect(0, 0, rect().w, rect().h));
}
