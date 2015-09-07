// (C) 2015 Tim Gurto

#include "CheckBox.h"
#include "Label.h"

extern Renderer renderer;

const int CheckBox::BOX_SIZE = 8;
const int CheckBox::GAP = 3;
const int CheckBox::Y_OFFSET = 1;

CheckBox::CheckBox(const SDL_Rect &rect, bool &linkedBool, const std::string &caption,
                   bool inverse):
Element(rect),
_depressed(false),
_mouseButtonDown(false),
_linkedBool(linkedBool),
_inverse(inverse){
    if (!caption.empty())
        addChild(new Label(makeRect(BOX_SIZE + GAP, 0, rect.w, rect.h),
                           caption, LEFT_JUSTIFIED, CENTER_JUSTIFIED));
    setMouseDownFunction(&mouseDown);
    setMouseUpFunction(&mouseUp);
    setMouseMoveFunction(&mouseMove);
}

void CheckBox::toggleBool(){
    _linkedBool = !_linkedBool;
    _lastCheckedValue = _linkedBool;
}

void CheckBox::depress(){
    _depressed = true;
    markChanged();
}

void CheckBox::release(bool click){
    if (click)
        toggleBool();
    markChanged();
    _depressed = false;
}

void CheckBox::mouseDown(Element &e, const Point &mousePos){
    CheckBox &checkBox = dynamic_cast<CheckBox&>(e);
    checkBox._mouseButtonDown = true;
    checkBox.depress();
}

void CheckBox::mouseUp(Element &e, const Point &mousePos){
    CheckBox &checkBox = dynamic_cast<CheckBox&>(e);
    checkBox._mouseButtonDown = false;
    if (checkBox._depressed) {
        bool click = collision(mousePos, makeRect(0, 0, checkBox.rect().w, checkBox.rect().h));
        checkBox.release(click);
    }
}

void CheckBox::mouseMove(Element &e, const Point &mousePos){
    CheckBox &checkBox = dynamic_cast<CheckBox&>(e);
    if (collision(mousePos, makeRect(0, 0, checkBox.rect().w, checkBox.rect().h))) {
        if (checkBox._mouseButtonDown && !checkBox._depressed)
            checkBox.depress();
    } else {
        if (checkBox._depressed)
            checkBox.release(false);
    }
}

void CheckBox::checkIfChanged(){
    if (_lastCheckedValue != _linkedBool) {
        _lastCheckedValue = _linkedBool;
        markChanged();
    }
    Element::checkIfChanged();
}

void CheckBox::refresh(){
    renderer.pushRenderTarget(_texture);

    makeBackgroundTransparent();

    // Box
    SDL_Rect boxRect = makeRect(0, (rect().h - BOX_SIZE) / 2 + Y_OFFSET, BOX_SIZE, BOX_SIZE);
    renderer.setDrawColor(FONT_COLOR);
    renderer.drawRect(boxRect);

    // Check (filled square)
    if (_depressed || (_linkedBool ^ _inverse)) {
        if (_depressed)
            renderer.setDrawColor(SHADOW_LIGHT);
        static const SDL_Rect CHECK_OFFSET_RECT = makeRect(2, 2, -4, -4);
        renderer.fillRect(boxRect + CHECK_OFFSET_RECT);
    }

    drawChildren();

    renderer.popRenderTarget();
}
