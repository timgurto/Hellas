#include "ChoiceList.h"

#include <cassert>

static const std::string EMPTY_STR = "";

extern Renderer renderer;

ChoiceList::ChoiceList(const ScreenRect &rect, px_t childHeight, Client &client)
    : List(rect, childHeight),
      _selectedBox(
          new ShadowBox({0, 0, rect.w - List::ARROW_W, childHeight}, true)),
      _mouseOverBox(new ShadowBox({0, 0, rect.w - List::ARROW_W, childHeight})),
      _mouseDownBox(
          new ShadowBox({0, 0, rect.w - List::ARROW_W, childHeight}, true)),
      _boxLayer(new Element(_content->rect())) {
  setClient(client);

  setLeftMouseDownFunction(markMouseDown);
  setLeftMouseUpFunction(toggle);
  setMouseMoveFunction(markMouseOver);

  _selectedBox->hide();
  _mouseOverBox->hide();
  _mouseDownBox->hide();
  _boxLayer->addChild(_selectedBox);
  _boxLayer->addChild(_mouseOverBox);
  _boxLayer->addChild(_mouseDownBox);
  Element::addChild(_boxLayer);
}

const std::string &ChoiceList::getIdFromMouse(double mouseY, int *index) const {
  int i = static_cast<int>((mouseY - _content->rect().y) / childHeight());
  if (i < 0 || i >= static_cast<int>(_content->children().size())) {
    *index = -1;
    return EMPTY_STR;
  }
  *index = i;
  auto it = _content->children().cbegin();
  while (i-- > 0) ++it;
  return (*it)->id();
}

bool ChoiceList::contentCollision(const ScreenPoint &p) const {
  return collision(p, ScreenRect{0, 0, _content->rect().w,
                                 min(_content->rect().h, rect().h)});
}

void ChoiceList::markMouseDown(Element &e, const ScreenPoint &mousePos) {
  ChoiceList &list = dynamic_cast<ChoiceList &>(e);
  if (!list.contentCollision(mousePos)) {
    list._mouseDownBox->hide();
    list.markChanged();
    return;
  }
  int index;
  list._mouseDownID = list.getIdFromMouse(mousePos.y, &index);
  if (index < 0) {
    list._mouseDownBox->hide();
    list.markChanged();
    return;
  }
  list._mouseDownBox->setPosition(0, index * list.childHeight());
  list._mouseDownBox->show();
  list.markChanged();
}

void ChoiceList::toggle(Element &e, const ScreenPoint &mousePos) {
  ChoiceList &list = dynamic_cast<ChoiceList &>(e);
  List::mouseUp(e, mousePos);
  if (list._mouseDownID == EMPTY_STR) {
    if (list.onSelect != nullptr) list.onSelect(*list.client());
    return;
  }
  if (!list.contentCollision(mousePos)) {
    list._mouseDownBox->hide();
    return;
  }
  int index;
  const std::string &id = list.getIdFromMouse(mousePos.y, &index);
  if (index < 0) {
    list._selectedID = EMPTY_STR;
    list._selectedBox->hide();
    list._mouseDownID = EMPTY_STR;
    list._mouseDownBox->hide();
    return;
  }
  if (list._mouseDownID !=
      id) {  // Mouse was moved away before releasing button
    list._mouseDownID = EMPTY_STR;
    list._mouseDownBox->hide();
    list.markChanged();
    return;
  }

  if (list._selectedID == id) {  // Unselect the current item
    list._selectedID = EMPTY_STR;
    list._selectedBox->hide();
  } else {
    list._selectedID = id;
    list._selectedBox->setPosition(0, index * list.childHeight());
    list._selectedBox->show();
    list._mouseOverBox->hide();
  }
  list._mouseDownID = EMPTY_STR;
  list._mouseDownBox->hide();
  list.markChanged();

  if (list.onSelect != nullptr) list.onSelect(*list.client());
}

void ChoiceList::markMouseOver(Element &e, const ScreenPoint &mousePos) {
  ChoiceList &list = dynamic_cast<ChoiceList &>(e);
  if (!list.contentCollision(mousePos)) {
    if (list._mouseOverID != EMPTY_STR) {
      list._mouseOverID = EMPTY_STR;
      list._mouseOverBox->hide();
    }
    return;
  }
  int index;
  list._mouseOverID = list.getIdFromMouse(mousePos.y, &index);
  if (index < 0) {
    list._mouseOverID = EMPTY_STR;
    list._mouseOverBox->hide();
    return;
  }
  px_t itemY = index * list.childHeight();
  if (list._mouseOverID == list._mouseDownID) {
    list._mouseDownBox->setPosition(0, itemY);
    list._mouseDownBox->show();
  } else {
    list._mouseDownBox->hide();
  }
  if (list._mouseOverID == list._selectedID ||
      list._mouseOverID == list._mouseDownID) {
    list._mouseOverBox->hide();
  } else {
    list._mouseOverBox->setPosition(0, itemY);
    list._mouseOverBox->show();
  }
}

void ChoiceList::manuallySelect(const std::string &id) {
  _selectedID = id;
  _selectedBox->hide();
  onSelect(*client());
}

void ChoiceList::verifyBoxes() {
  bool mouseOverFound = _mouseOverID == EMPTY_STR,
       mouseDownFound = _mouseDownID == EMPTY_STR,
       selectedFound = _selectedID == EMPTY_STR;

  for (const Element *item : _content->children()) {
    const std::string &id = item->id();
    if (!mouseOverFound && id == _mouseOverID) {
      mouseOverFound = true;
      if (item->rect().y != _mouseOverBox->rect().y)
        _mouseOverBox->rect(item->rect());
    }
    if (!mouseDownFound && id == _mouseDownID) {
      mouseDownFound = true;
      if (item->rect().y != _mouseDownBox->rect().y)
        _mouseDownBox->rect(item->rect());
    }
    if (!selectedFound && id == _selectedID) {
      selectedFound = true;
      if (item->rect().y != _selectedBox->rect().y)
        _selectedBox->rect(item->rect());
    }
  }

  if (!mouseOverFound) {
    _mouseOverID = EMPTY_STR;
    _mouseOverBox->hide();
  }
  if (!mouseDownFound) {
    _mouseDownID = EMPTY_STR;
    _mouseDownBox->hide();
  }
  if (!selectedFound) {
    _selectedID = EMPTY_STR;
    _selectedBox->hide();
  }
}

void ChoiceList::refresh() {
  List::refresh();
  _boxLayer->rect(_content->rect());
}

void ChoiceList::clearSelection() {
  _selectedID = "";
  _selectedBox->hide();
  _mouseDownBox->hide();
  markChanged();
}
