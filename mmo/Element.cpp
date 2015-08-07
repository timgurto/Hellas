// (C) 2015 Tim Gurto

#include "Element.h"

extern Renderer renderer;

const Color Element::BACKGROUND_COLOR = Color::GREY_4;
const Color Element::FONT_COLOR = Color::WHITE;
TTF_Font *Element::_font = 0;

Element::Element(const SDL_Rect &rect):
_rect(rect),
_texture(rect.w, rect.h),
_changed(true){}

void Element::refresh(){
    renderer.pushRenderTarget(_texture);
    drawChildren();
    renderer.popRenderTarget();
}

Element::~Element(){
    // Free children
    for (std::vector<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
}

void Element::drawChildren() const{
    for (std::vector<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if ((*it)->_changed)
            _changed = true;

        (*it)->draw();
    }
}

void Element::draw(){
    if (_changed) {
        refresh();
        _changed = false;
    }
    _texture.draw(_rect);
}
