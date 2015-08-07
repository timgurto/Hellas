// (C) 2015 Tim Gurto

#include "Label.h"

extern Renderer renderer;

Label::Label(const SDL_Rect &rect, const std::string &text,
             Justification justificationH, Justification justificationV):
Element(rect),
_text(text),
_justificationH(justificationH),
_justificationV(justificationV){}

void Label::refresh(){
    renderer.pushRenderTarget(_texture);

    makeBackgroundTransparent();

    Texture text(font(), _text, FONT_COLOR);

    int x = 0;
    switch(_justificationH) {
    case RIGHT_JUSTIFIED:
        x = rect().w - text.width();
        break;
    case CENTER_JUSTIFIED:
        x = (rect().w - text.width()) / 2;
        break;
    }

    int y = 0;
    switch (_justificationV) {
    case BOTTOM_JUSTIFIED:
        y = rect().h - text.height();
        break;
    case CENTER_JUSTIFIED:
        y = (rect().h - text.height()) / 2;
        break;
    }

    text.draw(x, y);

    drawChildren();

    renderer.popRenderTarget();
}
