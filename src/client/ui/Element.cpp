// (C) 2015 Tim Gurto

#include <cassert>

#include "Element.h"

extern Renderer renderer;

bool Element::initialized = false;

Color Element::BACKGROUND_COLOR;
Color Element::SHADOW_LIGHT;
Color Element::SHADOW_DARK;
Color Element::FONT_COLOR;

TTF_Font *Element::_font = 0;
int Element::textOffset = 0;

Texture Element::transparentBackground;

const Point *Element::absMouse = 0;

Element::Element(const Rect &rect):
_changed(true),
_dimensionsChanged(false),
_visible(true),
_rect(rect),
_parent(0),
_texture(rect.w, rect.h),
_mouseDown(0), _mouseDownElement(0),
_mouseUp(0), _mouseUpElement(0),
_mouseMove(0), _mouseMoveElement(0),
_scrollUp(0), _scrollUpElement(0),
_scrollDown(0), _scrollDownElement(0),
_preRefresh(0), _preRefreshElement(0){
    if (!initialized) {
        BACKGROUND_COLOR = Color::GREY_4;
        SHADOW_LIGHT = Color::GREY_2;
        SHADOW_DARK = Color::GREY_8;
        FONT_COLOR = Color::WHITE;
        initialized = true;
    }

    _texture.setBlend(SDL_BLENDMODE_BLEND);
}

Element::~Element(){
    clearChildren();
}

void Element::rect(int x, int y, int w, int h){
    _rect = Rect(x, y, w, h);
    _dimensionsChanged = true;
}

void Element::rect(const Rect &rhs){
    _rect = rhs;
    _dimensionsChanged = true;
}

void Element::rect(int x, int y) {
    _rect.x = x;
    _rect.y = y;
    if (_parent)
        _parent->markChanged();
}

void Element::width(int w){
    _rect.w = w;
    _dimensionsChanged = true;
}

void Element::height(int h){
    _rect.h = h;
    _dimensionsChanged = true;
}

void Element::markChanged() const{
    _changed = true;
    if (_parent)
        _parent->markChanged();
}

void Element::show(){
    if (!_visible && _parent)
        _parent->markChanged();
    _visible = true;
}

void Element::hide(){
    if (_visible && _parent)
        _parent->markChanged();
    _visible = false;
}

void Element::toggleVisibility(){
    _visible = !_visible;
    if (_parent)
        _parent->markChanged();
}

void Element::drawChildren() const{
    for (Element *child : _children) {
        if (child->_changed)
            markChanged();

        child->draw();
    }
}

void Element::checkIfChanged(){
    for (Element *child : _children)
        child->checkIfChanged();
}

bool Element::onMouseDown(const Point &mousePos){
    // Assumption: if this is called, then the mouse collides with the element.
    // Assumption: each element has at most one child that collides with the mouse.
    if (!_visible)
        return false;
    const Point relativeLocation = mousePos - location();
    bool functionCalled = false;
    for (Element *child : _children) {
        if (collision(relativeLocation, child->rect())) {
            if (child->onMouseDown(relativeLocation)) {
                functionCalled = true;
            }
        }
    }
    if (functionCalled)
        return true;
    /*
    If execution gets here, then this element has no children that both collide
    and have _mouseDown defined.
    */
    if (_mouseDown) {
        _mouseDown(*_mouseDownElement, relativeLocation);
        return true;
    } else
        return false;
}

bool Element::onScrollUp(const Point &mousePos){
    // Assumption: if this is called, then the mouse collides with the element.
    // Assumption: each element has at most one child that collides with the mouse.
    if (!_visible)
        return false;
    const Point relativeLocation = mousePos - location();
    bool functionCalled = false;
    for (Element *child : _children) {
        if (collision(relativeLocation, child->rect())) {
            if (child->onScrollUp(relativeLocation)) {
                functionCalled = true;
            }
        }
    }
    if (functionCalled)
        return true;
    /*
    If execution gets here, then this element has no children that both collide
    and have _scrollUp defined.
    */
    if (_scrollUp) {
        _scrollUp(*_scrollUpElement);
        return true;
    } else
        return false;
}

bool Element::onScrollDown(const Point &mousePos){
    // Assumption: if this is called, then the mouse collides with the element.
    // Assumption: each element has at most one child that collides with the mouse.
    if (!_visible)
        return false;
    const Point relativeLocation = mousePos - location();
    bool functionCalled = false;
    for (Element *child : _children) {
        if (collision(relativeLocation, child->rect())) {
            if (child->onScrollDown(relativeLocation)) {
                functionCalled = true;
            }
        }
    }
    if (functionCalled)
        return true;
    /*
    If execution gets here, then this element has no children that both collide
    and have _scrollDown defined.
    */
    if (_scrollDown) {
        _scrollDown(*_scrollDownElement);
        return true;
    } else
        return false;
}

void Element::onMouseUp(const Point &mousePos){
    if (!_visible)
        return;
    const Point relativeLocation = mousePos - location();
    if (_mouseUp)
        _mouseUp(*_mouseUpElement, relativeLocation);
    for (Element *child : _children)
        child->onMouseUp(relativeLocation);
}

void Element::onMouseMove(const Point &mousePos){
    if (!_visible)
        return;
    const Point relativeLocation = mousePos - location();
    if (_mouseMove)
        _mouseMove(*_mouseMoveElement, relativeLocation);
    for (Element *child : _children)
        child->onMouseMove(relativeLocation);
}

void Element::setMouseDownFunction(mouseDownFunction_t f, Element *e){
    _mouseDown = f;
    _mouseDownElement = e ? e : this;
}

void Element::setMouseUpFunction(mouseUpFunction_t f, Element *e){
    _mouseUp = f;
    _mouseUpElement = e ? e : this;
}

void Element::setMouseMoveFunction(mouseMoveFunction_t f, Element *e){
    _mouseMove = f;
    _mouseMoveElement = e ? e : this;
}

void Element::setScrollUpFunction(scrollUpFunction_t f, Element *e){
    _scrollUp = f;
    _scrollUpElement = e ? e : this;
}

void Element::setScrollDownFunction(scrollDownFunction_t f, Element *e){
    _scrollDown = f;
    _scrollDownElement = e ? e : this;
}

void Element::setPreRefreshFunction(preRefreshFunction_t f, Element *e){
    _preRefresh = f;
    _preRefreshElement = e ? e : this;
}

// Note: takes ownership of child.  Child should be allocated with new.
void Element::addChild(Element *child){
    assert(child);
    _children.push_back(child);
    child->_parent = this;
    markChanged();
}

void Element::clearChildren(){
    for (Element *child : _children)
        delete child;
    _children.clear();
    markChanged();
}

Element *Element::findChild(const std::string id){
    // Check this level first
    for (Element *child : _children)
        if (child->_id == id)
            return child;

    // Check children recursively
    for (Element *child : _children) {
        Element *found = child->findChild(id);
        if (found)
            return found;
    }
    return 0;
}

void Element::refresh(){
    if (!_texture)
        return;

    renderer.pushRenderTarget(_texture);

    drawChildren();

    renderer.popRenderTarget();
}

void Element::draw(){
    if (!_visible)
        return;
    if (!_parent)
        checkIfChanged();
    if (_dimensionsChanged) {
        _texture = Texture(_rect.w, _rect.h);
        markChanged();
        _dimensionsChanged = false;
    }
    if (true || !_texture)
        markChanged();
    if (_changed) {
        if (_preRefresh)
            _preRefresh(*_preRefreshElement);
        refresh();
        _changed = false;
    }
    _texture.setBlend(SDL_BLENDMODE_BLEND);
    _texture.draw(_rect);
}

void Element::forceRefresh(){
    if (!_parent)
        transparentBackground = Texture();
    _changed = true;
    for (Element *child : _children)
        child->forceRefresh();
}

void Element::makeBackgroundTransparent(){
    if (!transparentBackground) {
        transparentBackground = Texture(4,4);//Texture(renderer.width(), renderer.height());
        transparentBackground.setAlpha(0);
        transparentBackground.setBlend(SDL_BLENDMODE_NONE);
        renderer.pushRenderTarget(transparentBackground);
        renderer.fill();
        renderer.popRenderTarget();
    }
    Rect dstRect = Rect(0, 0, _rect.w, _rect.h);
    transparentBackground.draw(dstRect);
}

void Element::cleanup(){
    transparentBackground = Texture();
}
