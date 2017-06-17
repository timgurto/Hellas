#include "Button.h"
#include "ColorBlock.h"
#include "Label.h"

extern Renderer renderer;

Button::Button(const Rect &rect, const std::string &caption, clickFun_t clickFunction,
               void *clickData):
Element(rect),
_content(new Element(Rect(0, 0, rect.w, rect.h))),
_clickFun(clickFunction),
_clickData(clickData),
_mouseButtonDown(false),
_depressed(false){
    Element::addChild(new ColorBlock(Rect(1, 1, rect.w - 2, rect.h - 2)));
    Element::addChild(_content);
    _shadowBox = new ShadowBox(Rect(0, 0, rect.w, rect.h));
    Element::addChild(_shadowBox);

    if (!caption.empty())
        addChild(new Label(Rect(0, 0, rect.w, rect.h),
                           caption, CENTER_JUSTIFIED, CENTER_JUSTIFIED));
    setLeftMouseDownFunction(&mouseDown);
    setLeftMouseUpFunction(&mouseUp);
    setMouseMoveFunction(&mouseMove);
}

void Button::depress(){
    _shadowBox->setReversed(true);
    _content->setPosition(1, 1); // Draw contents at an offset
    _depressed = true;
    markChanged();
}

void Button::release(bool click){
    _shadowBox->setReversed(false);
    _content->setPosition(0, 0);
    if (click && _clickFun != nullptr)
        _clickFun(_clickData);
    _depressed = false;
    markChanged();
}

void Button::mouseDown(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    button._mouseButtonDown = true;
    button.depress();
}

void Button::mouseUp(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    button._mouseButtonDown = false;
    if (button._depressed) {
        bool click = collision(mousePos, Rect(0, 0, button.rect().w, button.rect().h));
        button.release(click);
    }
}

void Button::mouseMove(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
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

Element *Button::findChild(const std::string id){
    return _content->findChild(id);
}
