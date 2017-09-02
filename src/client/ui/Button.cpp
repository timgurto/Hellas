#include "Button.h"
#include "ColorBlock.h"
#include "ShadowBox.h"
#include "Label.h"

extern Renderer renderer;

Button::Button(const Rect &rect, const std::string &caption, clickFun_t clickFunction,
               void *clickData):
Element(rect),
_content(new Element(0)),
_background(new ColorBlock(Rect(1, 1, 0, 0))),
_border(new ShadowBox(0)),
_caption(nullptr),
_clickFun(clickFunction),
_clickData(clickData),
_mouseButtonDown(false),
_depressed(false),
_enabled(true){
    Element::addChild(_background);
    Element::addChild(_content);
    Element::addChild(_border);

    if (!caption.empty()){
        _caption = new Label(Rect(0, 0, rect.w, rect.h), caption,
                CENTER_JUSTIFIED, CENTER_JUSTIFIED);
        addChild(_caption);
    }

    width(rect.w);
    height(rect.h);

    setLeftMouseDownFunction(&mouseDown);
    setLeftMouseUpFunction(&mouseUp);
    setMouseMoveFunction(&mouseMove);
}

void Button::depress(){
    _border->setReversed(true);
    _content->setPosition(1, 1); // Draw contents at an offset
    _depressed = true;
    markChanged();
}

void Button::release(bool click){
    _border->setReversed(false);
    _content->setPosition(0, 0);
    if (click && _clickFun != nullptr)
        _clickFun(_clickData);
    _depressed = false;
    markChanged();
}

void Button::mouseDown(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    if (!button._enabled)
        return;

    button._mouseButtonDown = true;
    button.depress();
}

void Button::mouseUp(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    if (!button._enabled)
        return;

    button._mouseButtonDown = false;
    if (button._depressed) {
        bool click = collision(mousePos, Rect(0, 0, button.rect().w, button.rect().h));
        button.release(click);
    }
}

void Button::mouseMove(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    if (!button._enabled)
        return;

    if (collision(mousePos, Rect(0, 0, button.rect().w, button.rect().h))) {
        if (button._mouseButtonDown && !button._depressed)
            button.depress();
    } else {
        if (button._depressed)
            button.release(false);
    }
}

void Button::addChild(Element *child){
    _content->addChild(child);
}

void Button::clearChildren(){
    _content->clearChildren();
    markChanged();
}

Element *Button::findChild(const std::string id) const{
    return _content->findChild(id);
}

void Button::width(px_t w){
    Element::width(w);
    _background->width(w - 2);
    _content->width(w);
    _border->width(w);
    if (_caption != nullptr)
        _caption->width(w);
}

void Button::height(px_t h){
    Element::height(h);
    _background->height(h - 2);
    _content->height(h);
    _border->height(h);
    if (_caption != nullptr)
        _caption->height(h);
}

void Button::enable(){
    _enabled = true;
    _caption->setColor(FONT_COLOR);
}

void Button::disable(){
    _enabled = false;
    _caption->setColor(Color::DISABLED_TEXT);
}
