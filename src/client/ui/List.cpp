#include "List.h"

List::List(const ScreenRect &rect, px_t childHeight)
    : Scrollable(rect), _childHeight(childHeight) {
  if (_childHeight <= 0)  // Prevent div/0
    _childHeight = 1;
}

void List::addChild(Element *child) {
  child->rect(0, _content->height(), rect().w - ARROW_W, _childHeight);
  Scrollable::addChild(child);
}

void List::addGap() { addChild(new Element{{}}); }
