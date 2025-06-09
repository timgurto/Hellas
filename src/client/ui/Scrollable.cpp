#include "Scrollable.h"

#include "../Client.h"
#include "../Renderer.h"
#include "ColorBlock.h"
#include "Label.h"
#include "Line.h"
#include "Scrollable.h"
#include "ShadowBox.h"

extern Renderer renderer;

const px_t Scrollable::ARROW_W = 8;
const px_t Scrollable::ARROW_H = 5;
const px_t Scrollable::CURSOR_HEIGHT = 8;
const px_t Scrollable::SCROLL_AMOUNT = 10;

Scrollable::Scrollable(const ScreenRect &rect)
    : Element(rect),
      _scrollBar(new Element({rect.w - ARROW_W, 0, ARROW_W, rect.h})),
      _cursor(new Element({0, 0, ARROW_W, CURSOR_HEIGHT})),
      _content(new Element({0, 0, rect.w - ARROW_W, 0})) {
  Element::addChild(_content);
  Element::addChild(_scrollBar);

  // Scroll bar details
  _scrollBar->addChild(
      new Line({ARROW_W / 2 - 1, ARROW_H}, rect.h - 2 * ARROW_H, VERTICAL));

  _whiteUp =
      new Picture({0, 0, ARROW_W, ARROW_H}, Client::images.scrollArrowWhiteUp);
  _greyUp =
      new Picture({0, 0, ARROW_W, ARROW_H}, Client::images.scrollArrowGreyUp);
  _whiteDown = new Picture({0, rect.h - ARROW_H, ARROW_W, ARROW_H},
                           Client::images.scrollArrowWhiteDown);
  _greyDown = new Picture({0, rect.h - ARROW_H, ARROW_W, ARROW_H},
                          Client::images.scrollArrowGreyDown);

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

void Scrollable::updateScrollBar() {
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

void Scrollable::refresh() {
  // Hide scroll bar if not needed
  if (_content->rect().h <= rect().h)
    _scrollBar->hide();
  else
    _scrollBar->show();
}

void Scrollable::addChild(Element *child) {
  auto childBottom = child->rect().y + child->rect().h;
  if (childBottom > _content->height()) _content->height(childBottom);

  _content->addChild(child);
}

void Scrollable::clearChildren() {
  _content->clearChildren();
  _content->height(0);
  if (_shouldScrollToTopOnClear)
    scrollPos(0);
  else
    ;  // TODO: make sure it's either scrolled to the top, or longer than its
       // space.
  markChanged();
}

Element *Scrollable::findChild(const std::string id) const {
  return _content->findChild(id);
}

void Scrollable::cursorMouseDown(Element &e, const ScreenPoint &mousePos) {
  auto &scrollable = dynamic_cast<Scrollable &>(e);
  scrollable._mouseDownOnCursor = true;
  scrollable._cursorOffset = toInt(mousePos.y);
}

void Scrollable::mouseUp(Element &e, const ScreenPoint &) {
  auto &scrollable = dynamic_cast<Scrollable &>(e);
  scrollable._mouseDownOnCursor = false;
}

void Scrollable::mouseMove(Element &e, const ScreenPoint &mousePos) {
  auto &scrollable = dynamic_cast<Scrollable &>(e);
  if (scrollable._mouseDownOnCursor) {
    // Scroll based on mouse pos
    scrollable._scrolledToBottom = false;
    static const px_t Y_MIN = ARROW_H + scrollable._cursorOffset - 1;
    const px_t Y_MAX = scrollable._scrollBar->rect().h - ARROW_H -
                       CURSOR_HEIGHT + scrollable._cursorOffset + 1;
    const px_t Y_RANGE = Y_MAX - Y_MIN;
    double progress = (mousePos.y - Y_MIN) / Y_RANGE;
    if (progress < 0)
      progress = 0;
    else if (progress >= 1) {
      progress = 1;
      scrollable._scrolledToBottom = true;
    }
    px_t newY = toInt(-progress *
                      (scrollable._content->rect().h - scrollable.rect().h));
    scrollable._content->setPosition(0, newY);
    scrollable.updateScrollBar();
  }
}

void Scrollable::scrollUpRaw(Element &e) {
  auto &scrollable = dynamic_cast<Scrollable &>(e);
  if (scrollable._content->rect().h <= scrollable.rect().h) return;
  scrollable._scrolledToBottom = false;
  scrollable._content->setPosition(
      0, scrollable._content->rect().y + SCROLL_AMOUNT);
  if (scrollable._content->rect().y > 0) scrollable._content->setPosition(0, 0);
  scrollable.updateScrollBar();
}

void Scrollable::scrollDownRaw(Element &e) {
  auto &scrollable = dynamic_cast<Scrollable &>(e);
  if (scrollable._content->rect().h <= scrollable.rect().h) return;
  scrollable._scrolledToBottom = false;
  scrollable._content->setPosition(
      0, scrollable._content->rect().y - SCROLL_AMOUNT);
  const px_t minScroll = -(scrollable._content->rect().h - scrollable.rect().h);
  if (scrollable._content->rect().y <= minScroll) {
    scrollable._content->setPosition(0, minScroll);
    scrollable._scrolledToBottom = true;
  }
  scrollable.updateScrollBar();
}

void Scrollable::scrollToTop() {
  _content->setPosition(0, 0);
  updateScrollBar();
}

void Scrollable::scrollToBottom() {
  if (_content->rect().h <= rect().h) return;
  const px_t minScroll = -(_content->rect().h - rect().h);
  _content->setPosition(0, minScroll);
  _scrolledToBottom = true;
  updateScrollBar();
}

bool Scrollable::isScrolledPastBottom() const {
  if (_content->rect().y >= 0) return false;
  const auto bottom = _content->rect().y + _content->rect().h;
  return bottom < height();
}

void Scrollable::scrollPos(px_t newPos) {
  _content->setPosition(0, newPos);
  updateScrollBar();
}
