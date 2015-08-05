// (C) 2015 Tim Gurto

#include "Element.h"
#include "Renderer.h"

extern Renderer renderer;

const Color Element::BACKGROUND_COLOR = Color::GREY_4;
const Color Element::FONT_COLOR = Color::WHITE;
TTF_Font *Element::_font = 0;

Element::Element():
_changed(true){}

Element::Element(const SDL_Rect &rect):
_rect(rect),
_texture(rect.w, rect.h),
_changed(true){}

void Element::refresh() const{
    _texture.setRenderTarget();
    drawChildren();
    renderer.setRenderTarget();
}

void Element::drawChildren() const{
    for (std::vector<Element>::const_iterator it = _children.begin(); it != _children.end(); ++it)
    it->draw();
}

void Element::draw() const{
    if (_changed) {
        refresh();
        _changed = false;
    }
    _texture.draw(_rect);
}
