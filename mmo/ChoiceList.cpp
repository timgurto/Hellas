// (C) 2015 Tim Gurto

#include <cassert>

#include "ChoiceList.h"

static const std::string EMPTY_STR = "";

extern Renderer renderer;

ChoiceList::ChoiceList(const SDL_Rect &rect, int childHeight):
List(rect, childHeight),
_selectedBox(new ShadowBox(makeRect(0, 0, rect.w - List::ARROW_W, childHeight), true)),
_mouseDownBox(new ShadowBox(makeRect(0, 0, rect.w - List::ARROW_W, childHeight), true)),
_mouseOverBox(new ShadowBox(makeRect(0, 0, rect.w - List::ARROW_W, childHeight))){
    setMouseDownFunction(markMouseDown);
    setMouseUpFunction(toggle);
    setMouseMoveFunction(markMouseOver);

    _selectedBox->hide();
    _mouseOverBox->hide();
    _mouseDownBox->hide();
    Element::addChild(_selectedBox);
    Element::addChild(_mouseOverBox);
    Element::addChild(_mouseDownBox);
}

const std::string &ChoiceList::getIdFromMouse(double mouseY, int *index) const{
    int i = static_cast<int>(mouseY / childHeight());
    if (i < 0 || i >= static_cast<int>(_content->children().size())) {
        *index = -1;
        return EMPTY_STR;
    }
    *index = i;
    auto it = _content->children().cbegin();
    while (i-- > 0)
        ++it;
    return (*it)->id();
}

bool ChoiceList::contentCollision(const Point &p) const{
    return collision(p, makeRect(0,
                                 0,
                                 _content->rect().w,
                                 min(_content->rect().h, rect().h)));
}

void ChoiceList::markMouseDown(Element &e, const Point &mousePos){
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    if (!list.contentCollision(mousePos)) {
        list._mouseDownBox->hide();
        list.markChanged();
        return;
    }
    int index;
    list._mouseDownID = list.getIdFromMouse(mousePos.y, &index);
    if (index < 0){
        list._mouseDownBox->hide();
        list.markChanged();
        return;
    }
    list._mouseDownBox->rect(0, index * list.childHeight());
    list._mouseDownBox->show();
    list.markChanged();
}

void ChoiceList::toggle(Element &e, const Point &mousePos){
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    if (list._mouseDownID == EMPTY_STR)
        return;
    if (!list.contentCollision(mousePos)) {
        list._mouseDownBox->hide();
        list.markChanged();
        return;
    }
    int index;
    const std::string &id = list.getIdFromMouse(mousePos.y, &index);
    if (index < 0){
        list._selectedID = EMPTY_STR;
        list._selectedBox->hide();
        list._mouseDownID = EMPTY_STR;
        list._mouseDownBox->hide();
        return;
    }
    if (list._mouseDownID != id) { // Mouse was moved away before releasing button
        list._mouseDownID = EMPTY_STR;
        list._mouseDownBox->hide();
        list.markChanged();
        return;
    }

    if (list._selectedID == id) { // Unselect the current item
        list._selectedID = EMPTY_STR;
        list._selectedBox->hide();
    } else {
        list._selectedID = id;
        list._selectedBox->rect(0, index * list.childHeight());
        list._selectedBox->show();
        list._mouseOverBox->hide();
    }
    list._mouseDownID = EMPTY_STR;
    list._mouseDownBox->hide();
    list.markChanged();
}

void ChoiceList::markMouseOver(Element &e, const Point &mousePos){
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    if (!list.contentCollision(mousePos)) {
        if (list._mouseOverID != EMPTY_STR) {
            list._mouseOverID = EMPTY_STR;
            list._mouseOverBox->hide();
            list.markChanged();
        }
        return;
    }
    int index;
    list._mouseOverID = list.getIdFromMouse(mousePos.y, &index);
    if (index < 0) {
        list._mouseOverID = EMPTY_STR;
        list._mouseOverBox->hide();
        list.markChanged();
        return;
    }
    int itemY = index * list.childHeight();
    if (list._mouseOverID == list._mouseDownID) {
        list._mouseDownBox->rect(0, itemY);
        list._mouseDownBox->show();
    } else {
        list._mouseDownBox->hide();
    }
    if (list._mouseOverID == list._selectedID || list._mouseOverID == list._mouseDownID) {
        list._mouseOverBox->hide();
    } else {
        list._mouseOverBox->rect(0, itemY);
        list._mouseOverBox->show();
    }
    list.markChanged();
}
