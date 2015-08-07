// (C) 2015 Tim Gurto

#include "Element.h"

extern Renderer renderer;

const Color Element::BACKGROUND_COLOR = Color::GREY_4;
const Color Element::SHADOW_LIGHT = Color::GREY_2;
const Color Element::SHADOW_DARK = Color::GREY_8;
const Color Element::FONT_COLOR = Color::WHITE;
TTF_Font *Element::_font = 0;

Texture Element::transparentBackground;

Element::Element(const SDL_Rect &rect):
_rect(rect),
_texture(rect.w, rect.h),
_changed(true){
    _texture.setBlend(SDL_BLENDMODE_BLEND);
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

void Element::onMouseDown(const Point &mousePos){
    for (std::vector<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->onMouseDown(mousePos);
}

void Element::onMouseUp(const Point &mousePos){
    for (std::vector<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->onMouseUp(mousePos);
}

void Element::onMouseMove(const Point &mousePos){
    for (std::vector<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->onMouseMove(mousePos);
}

void Element::refresh(){
    renderer.pushRenderTarget(_texture);
    drawChildren();
    renderer.popRenderTarget();
}

void Element::draw(){
    if (_changed) {
        refresh();
        _changed = false;
    }
    _texture.draw(_rect);
}

void Element::makeBackgroundTransparent(){
    if (!transparentBackground) {
        transparentBackground = Texture(renderer.width(), renderer.height());
        transparentBackground.setAlpha(0);
        transparentBackground.setBlend(SDL_BLENDMODE_NONE);
        renderer.pushRenderTarget(transparentBackground);
        renderer.fill();
        renderer.popRenderTarget();
    }
    SDL_Rect rect = makeRect(0, 0,_rect.w, _rect.h);
    transparentBackground.draw(rect, rect);
}

void Element::cleanup(){
    transparentBackground = Texture();
}
