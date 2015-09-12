// (C) 2015 Tim Gurto

#include "Element.h"

extern Renderer renderer;

const Color Element::BACKGROUND_COLOR = Color::GREY_4;
const Color Element::SHADOW_LIGHT = Color::GREY_2;
const Color Element::SHADOW_DARK = Color::GREY_8;
const Color Element::FONT_COLOR = Color::WHITE;
TTF_Font *Element::_font = 0;

Texture Element::transparentBackground;

const Point *Element::absMouse = 0;

Element::Element(const SDL_Rect &rect):
_rect(rect),
_visible(true),
_texture(rect.w, rect.h),
_changed(true),
_dimensionsChanged(false),
_mouseDown(0), _mouseDownElement(0),
_mouseUp(0), _mouseUpElement(0),
_mouseMove(0), _mouseMoveElement(0),
_scrollUp(0), _scrollUpElement(0),
_scrollDown(0), _scrollDownElement(0),
_parent(0){
    _texture.setBlend(SDL_BLENDMODE_BLEND);
}

Element::~Element(){
    // Free children
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
}

void Element::rect(int x, int y, int w, int h){
    _rect = makeRect(x, y, w, h);
    _dimensionsChanged = true;
}

void Element::rect(const SDL_Rect &rhs){
    _rect = rhs;
    _dimensionsChanged = true;
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
    _visible = true;
    if (_parent)
        _parent->markChanged();
}

void Element::hide(){
    _visible = false;
    if (_parent)
        _parent->markChanged();
}

void Element::toggleVisibility(){
    _visible = !_visible;
    if (_parent)
        _parent->markChanged();
}

void Element::drawChildren() const{
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if ((*it)->_changed)
            markChanged();

        (*it)->draw();
    }
}

void Element::checkIfChanged(){
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->checkIfChanged();
}

bool Element::onMouseDown(const Point &mousePos){
    // Assumption: if this is called, then the mouse collides with the element.
    // Assumption: each element has at most one child that collides with the mouse.
    if (!_visible)
        return false;
    const Point relativeLocation = mousePos - location();
    bool functionCalled = false;
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if (collision(relativeLocation, (*it)->rect())) {
            if ((*it)->onMouseDown(relativeLocation)) {
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
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if (collision(relativeLocation, (*it)->rect())) {
            if ((*it)->onScrollUp(relativeLocation)) {
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
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if (collision(relativeLocation, (*it)->rect())) {
            if ((*it)->onScrollDown(relativeLocation)) {
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
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        if (collision(relativeLocation, (*it)->rect()))
            (*it)->onMouseUp(relativeLocation);
}

void Element::onMouseMove(const Point &mousePos){
    if (!_visible)
        return;
    const Point relativeLocation = mousePos - location();
    if (_mouseMove)
        _mouseMove(*_mouseMoveElement, relativeLocation);
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->onMouseMove(relativeLocation);
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

// Note: takes ownership of child.  Child should be allocated with new.
void Element::addChild(Element *child){
    assert(child);
    _children.push_back(child);
    child->_parent = this;
    markChanged();
};

void Element::clearChildren(){
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
    _children.clear();
    markChanged();
}

Element *Element::findChild(const std::string id){
    // Check this level first
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        if ((*it)->_id == id)
            return *it;

    // Check children recursively
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        Element *found = (*it)->findChild(id);
        if (found)
            return found;
    }
    return 0;
}

void Element::refresh(){
    renderer.pushRenderTarget(_texture);

    if (_solidBackground) {
        renderer.setDrawColor(BACKGROUND_COLOR);
        renderer.fill();
    }

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
    if (_changed) {
        refresh();
        _changed = false;
    }
    _texture.setBlend(SDL_BLENDMODE_BLEND);
    _texture.draw(_rect);
}

void Element::forceRefresh(){
    _changed = true;
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->forceRefresh();
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
    SDL_Rect dstRect = makeRect(0, 0, _rect.w, _rect.h);
    transparentBackground.draw(dstRect);
}

void Element::cleanup(){
    transparentBackground = Texture();
}
