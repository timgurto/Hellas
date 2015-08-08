// (C) 2015 Tim Gurto

#include "Button.h"
#include "Label.h"

extern Renderer renderer;

Button::Button(const SDL_Rect &rect, const std::string &caption, clickFun_t clickFunction,
               void *clickData):
Element(rect),
_mouseButtonDown(false),
_depressed(false),
_content(new Element(makeRect(0, 0, rect.w, rect.h))),
_clickFun(clickFunction),
_clickData(clickData){
    Element::addChild(_content);
    Element::addChild(new ShadowBox(makeRect(0, 0, rect.w, rect.h)));

    if (!caption.empty())
        addChild(new Label(makeRect(0, 0, rect.w, rect.h),
                           caption, CENTER_JUSTIFIED, CENTER_JUSTIFIED));
    setMouseDownFunction(&mouseDown);
    setMouseUpFunction(&mouseUp);
    setMouseMoveFunction(&mouseMove);
}

void Button::depress(){
    ShadowBox &border = *dynamic_cast<ShadowBox*>(* ++_children.begin());
    border.setReversed(true);
    _content->rect(1, 1); // Draw contents at an offset
    _depressed = true;
    markChanged();
}

void Button::release(bool click){
    ShadowBox &border = *dynamic_cast<ShadowBox*>(* ++_children.begin());
    border.setReversed(false);
    _content->rect(0, 0);
    if (click && _clickFun)
        _clickFun(_clickData);
    _depressed = false;
    markChanged();
}

void Button::mouseDown(Element &e){
    Button &button = dynamic_cast<Button&>(e);
    button._mouseButtonDown = true;
    button.depress();
}

void Button::mouseUp(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    button._mouseButtonDown = false;
    if (button._depressed) {
        bool click = collision(mousePos, makeRect(0, 0, button.rect().w, button.rect().h));
        button.release(click);
    }
}

void Button::mouseMove(Element &e, const Point &mousePos){
    Button &button = dynamic_cast<Button&>(e);
    if (collision(mousePos, makeRect(0, 0, button.rect().w, button.rect().h))) {
        if (button._mouseButtonDown && !button._depressed)
            button.depress();
    } else {
        if (button._depressed)
            button.release(false);
    }
}

void Button::refresh(){
    renderer.pushRenderTarget(_texture);

    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.fillRect(makeRect(1, 1, rect().w - 2, rect().h - 2));

    drawChildren();

    renderer.popRenderTarget();
}

void Button::addChild(Element *child){
    _content->addChild(child);
}

Element *Button::findChild(const std::string id){
    return _content->findChild(id);
}
