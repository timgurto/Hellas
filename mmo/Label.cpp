// (C) 2015 Tim Gurto

#include "Label.h"

extern Renderer renderer;

Label::Label(const SDL_Rect &rect, const std::string &text, Justification justification):
Element(rect),
_text(text),
_justification(justification){}

void Label::refresh(){
    renderer.pushRenderTarget(_texture);

    Texture text(font(), _text, FONT_COLOR);
    int x;
    switch(_justification) {
    case LEFT_JUSTIFIED:
        x = 0;
        break;
    case RIGHT_JUSTIFIED:
        x = rect().w - text.width();
        break;
    case CENTER_JUSTIFIED:
        x = (rect().w - text.width()) / 2;
    }
    text.draw(x, 0);

    drawChildren();

    renderer.popRenderTarget();
}
