// (C) 2015 Tim Gurto

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

void ChoiceList::refresh(){
    _mouseOverBox->hide();
    _mouseDownBox->hide();
    _selectedBox->hide();

    // Draw mouse-over and selection boxes
    for (std::list<Element*>::const_iterator it = _content->children().begin();
         it != _content->children().end(); ++it) {
        const std::string &id = (*it)->id();
        SDL_Rect itemRect = (*it)->rect();
        if (id == _selectedID) {
            _selectedBox->rect(itemRect.x, itemRect.y);
            _selectedBox->show();
        } else if (id == _mouseOverID) {
            if (id == _mouseDownID) {
                _mouseDownBox->rect(itemRect.x, itemRect.y);
                _mouseDownBox->show();
            } else {
                _mouseOverBox->rect(itemRect.x, itemRect.y);
                _mouseOverBox->show();
            }
        }
    }
    markChanged();

    List::refresh(); // Draw list elements and scroll bar
}

const std::string &ChoiceList::getIdFromMouse(double mouseY) const{
    int i = static_cast<int>(mouseY / childHeight());
    if (i < 0 || i >= static_cast<int>(_content->children().size()))
        return EMPTY_STR;
    std::list<Element*>::const_iterator it = _content->children().begin();
    while (i-- > 0)
        ++it;
    return (*it)->id();
}

void ChoiceList::markMouseDown(Element &e, const Point &mousePos){
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    if (!collision(mousePos, makeRect(0, 0, list._content->rect().w, list._content->rect().h)))
        return;
    list._mouseDownID = list.getIdFromMouse(mousePos.y);
    list.markChanged();
}

void ChoiceList::toggle(Element &e, const Point &mousePos){
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    if (list._mouseDownID == "")
        return;
    if (!collision(mousePos, makeRect(0, 0, list._content->rect().w, list._content->rect().h))) {
        list._mouseDownID = "";
        return;
    }
    const std::string &id = list.getIdFromMouse(mousePos.y);
    if (list._mouseDownID != id) { // Mouse was moved away before releasing button
        list._mouseDownID = "";
        return;
    }

    if (list._selectedID == id)
        list._selectedID = "";
    else
        list._selectedID = id;
    list._mouseDownID = "";
    list.markChanged();
}

void ChoiceList::markMouseOver(Element &e, const Point &mousePos){
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    if (!collision(mousePos, makeRect(0, 0, list._content->rect().w, list._content->rect().h))) {
        if (list._mouseOverID != "") {
            list._mouseOverID = "";
            list.markChanged();
        }
        return;
    }
    list._mouseOverID = list.getIdFromMouse(mousePos.y);
    list.markChanged();
}
