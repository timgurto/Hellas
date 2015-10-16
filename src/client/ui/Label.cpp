// (C) 2015 Tim Gurto

#include "Label.h"

extern Renderer renderer;

Label::Label(const Rect &rect, const std::string &text,
             Justification justificationH, Justification justificationV):
Element(rect),
_text(text),
_justificationH(justificationH),
_justificationV(justificationV),
_matchWidth(false){}

void Label::refresh(){
    renderer.pushRenderTarget(_texture);

    makeBackgroundTransparent();

    Texture text(font(), _text, FONT_COLOR);
    if (_matchWidth)
        width(text.width());

    int x;
    switch(_justificationH) {
    case RIGHT_JUSTIFIED:
        x = rect().w - text.width();
        break;
    case CENTER_JUSTIFIED:
        x = (rect().w - text.width()) / 2;
        break;
    case LEFT_JUSTIFIED:
    default:
        x = 0;
    }

    int y;
    switch (_justificationV) {
    case BOTTOM_JUSTIFIED:
        y = rect().h - text.height();
        break;
    case CENTER_JUSTIFIED:
        y = (rect().h - text.height()) / 2;
        break;
    case TOP_JUSTIFIED:
    default:
        y = 0;
    }

    text.draw(x, y);

    drawChildren();

    renderer.popRenderTarget();
}

void Label::matchW(){
    _matchWidth = true;
}
