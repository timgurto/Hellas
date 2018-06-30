#include "List.h"
#include "../Renderer.h"
#include "ColorBlock.h"
#include "Label.h"
#include "Line.h"
#include "ShadowBox.h"

extern Renderer renderer;

const px_t List::ARROW_W = 8;
const px_t List::ARROW_H = 5;
const px_t List::CURSOR_HEIGHT = 8;
const px_t List::SCROLL_AMOUNT = 10;

Texture List::whiteUp{}, List::whiteDown{}, List::greyUp{}, List::greyDown{};

List::List(const ScreenRect &rect, px_t childHeight)
    : Element(rect),
      _mouseDownOnCursor(false),
      _cursorOffset(0),
      _childHeight(childHeight),
      _scrollBar(new Element({rect.w - ARROW_W, 0, ARROW_W, rect.h})),
      _cursor(new Element({0, 0, ARROW_W, CURSOR_HEIGHT})),
      _scrolledToBottom(false),
      _content(new Element({0, 0, rect.w - ARROW_W, 0})) {
  if (_childHeight <= 0)  // Prevent div/0
    _childHeight = 1;
  // Element::addChild(new ColorBlock(Rect(0, 0, rect.w, rect.h)));
  Element::addChild(_content);
  Element::addChild(_scrollBar);

  // Scroll bar details
  _scrollBar->addChild(
      new Line(ARROW_W / 2 - 1, ARROW_H, rect.h - 2 * ARROW_H, VERTICAL));

  if (!whiteUp) {
    whiteUp = {"Images/arrowUp.png", Color::MAGENTA};
    greyUp = {"Images/arrowUpGrey.png", Color::MAGENTA};
    whiteDown = {"Images/arrowDown.png", Color::MAGENTA};
    greyDown = {"Images/arrowDownGrey.png", Color::MAGENTA};
  }
  _whiteUp = new Picture({0, 0, ARROW_W, ARROW_H}, whiteUp);
  _greyUp = new Picture({0, 0, ARROW_W, ARROW_H}, greyUp);
  _whiteDown = new Picture({0, rect.h - ARROW_H, ARROW_W, ARROW_H}, whiteDown);
  _greyDown = new Picture({0, rect.h - ARROW_H, ARROW_W, ARROW_H}, greyDown);

  _whiteUp->hide();
  _greyDown->hide();
  _whiteUp->setLeftMouseDownFunction(scrollUp, this);
  _whiteDown->setLeftMouseDownFunction(scrollDown, this);
  _scrollBar->addChild(_whiteUp);
  _scrollBar->addChild(_greyUp);
  _scrollBar->addChild(_whiteDown);
  _scrollBar->addChild(_greyDown);

  _cursor->setLeftMouseDownFunction(cursorMouseDown, this);
  setLeftMouseUpFunction(mouseUp, this);
  _scrollBar->setMouseMoveFunction(mouseMove, this);
  setScrollUpFunction(scrollUpRaw, this);
  setScrollDownFunction(scrollDownRaw, this);

  _cursor->addChild(
      new ColorBlock({1, 1, _cursor->width() - 2, _cursor->height() - 2}));
  _cursor->addChild(new ShadowBox(_cursor->rect()));
  _scrollBar->addChild(_cursor);
  updateScrollBar();
}

void List::updateScrollBar() {
  // Cursor position
  static const px_t Y_MIN = ARROW_H - 1;
  const px_t Y_MAX = _scrollBar->rect().h - ARROW_H - CURSOR_HEIGHT + 1;
  const px_t Y_RANGE = Y_MAX - Y_MIN;
  double progress = -1.0 * _content->rect().y / (_content->rect().h - rect().h);
  if (progress < 0)
    progress = 0;
  else if (progress >= 1.) {
    progress = 1.;
    _scrolledToBottom = true;
  }
  _cursor->setPosition(0, toInt(progress * Y_RANGE + Y_MIN));
  _scrollBar->markChanged();

  // Arrow visibility
  // TODO: use Picture::changeTexture() instead of hiding/showing.
  if (_content->rect().y == 0) {  // Scroll bar at top
    _whiteUp->hide();
    _greyUp->show();
    _whiteDown->show();
    _greyDown->hide();
  } else if (_scrolledToBottom) {
    _whiteUp->show();
    _greyUp->hide();
    _whiteDown->hide();
    _greyDown->show();
  } else {  // In the middle somewhere
    _whiteUp->show();
    _greyUp->hide();
    _whiteDown->show();
    _greyDown->hide();
  }
}

void List::refresh() {
  // Hide scroll bar if not needed
  if (_content->rect().h <= rect().h)
    _scrollBar->hide();
  else
    _scrollBar->show();
}

void List::addChild(Element *child) {
  child->rect(0, _content->height(), rect().w - ARROW_W, _childHeight);
  _content->height(_content->height() + _childHeight);
  _content->addChild(child);
}

void List::clearChildren() {
  _content->clearChildren();
  _content->height(0);
  if (_shouldScrollToTopOnClear)
    scrollPos(0);
  else
    ;  // TODO: make sure the list is either at the top, or longer than its
       // space.
  markChanged();
}

Element *List::findChild(const std::string id) const {
  return _content->findChild(id);
}

void List::cursorMouseDown(Element &e, const ScreenPoint &mousePos) {
  List &list = dynamic_cast<List &>(e);
  list._mouseDownOnCursor = true;
  list._cursorOffset = toInt(mousePos.y);
}

void List::mouseUp(Element &e, const ScreenPoint &mousePos) {
  List &list = dynamic_cast<List &>(e);
  list._mouseDownOnCursor = false;
}

void List::mouseMove(Element &e, const ScreenPoint &mousePos) {
  List &list = dynamic_cast<List &>(e);
  if (list._mouseDownOnCursor) {
    // Scroll based on mouse pos
    list._scrolledToBottom = false;
    static const px_t Y_MIN = ARROW_H + list._cursorOffset - 1;
    const px_t Y_MAX = list._scrollBar->rect().h - ARROW_H - CURSOR_HEIGHT +
                       list._cursorOffset + 1;
    const px_t Y_RANGE = Y_MAX - Y_MIN;
    double progress = (mousePos.y - Y_MIN) / Y_RANGE;
    if (progress < 0)
      progress = 0;
    else if (progress >= 1) {
      progress = 1;
      list._scrolledToBottom = true;
    }
    px_t newY = toInt(-progress * (list._content->rect().h - list.rect().h));
    list._content->setPosition(0, newY);
    list.updateScrollBar();
  }
}

void List::scrollUpRaw(Element &e) {
  List &list = dynamic_cast<List &>(e);
  if (list._content->rect().h <= list.rect().h) return;
  list._scrolledToBottom = false;
  list._content->setPosition(0, list._content->rect().y + SCROLL_AMOUNT);
  if (list._content->rect().y > 0) list._content->setPosition(0, 0);
  list.updateScrollBar();
}

void List::scrollDownRaw(Element &e) {
  List &list = dynamic_cast<List &>(e);
  if (list._content->rect().h <= list.rect().h) return;
  list._scrolledToBottom = false;
  list._content->setPosition(0, list._content->rect().y - SCROLL_AMOUNT);
  const px_t minScroll = -(list._content->rect().h - list.rect().h);
  if (list._content->rect().y <= minScroll) {
    list._content->setPosition(0, minScroll);
    list._scrolledToBottom = true;
  }
  list.updateScrollBar();
}

void List::scrollToBottom() {
  if (_content->rect().h <= rect().h) return;
  const px_t minScroll = -(_content->rect().h - rect().h);
  _content->setPosition(0, minScroll);
  _scrolledToBottom = true;
  updateScrollBar();
}

void List::scrollPos(px_t newPos) {
  _content->setPosition(0, newPos);
  updateScrollBar();
}
