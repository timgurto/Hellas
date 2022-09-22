#pragma once

#include "Element.h"
#include "Picture.h"

class Scrollable : public Element {
 public:
  static const px_t ARROW_W, ARROW_H, CURSOR_HEIGHT, SCROLL_AMOUNT;

  Scrollable(const ScreenRect &rect = {});

 protected:
  static void mouseUp(Element &e, const ScreenPoint &mousePos);

 private:
  bool _mouseDownOnCursor{false};
  px_t _cursorOffset{0};  // y-offset of the mouse on the cursor.

  Element *_scrollBar;
  Picture *_whiteUp{nullptr}, *_whiteDown{nullptr}, *_greyUp{nullptr},
      *_greyDown{nullptr};
  Element *_cursor{
      nullptr};  // Shows position on the scroll bar, and can be dragged.
  void updateScrollBar();  // Update the appearance of the scroll bar.
  bool _scrolledToBottom{false};

  bool _shouldScrollToTopOnClear = true;

  static void cursorMouseDown(Element &e, const ScreenPoint &mousePos);
  static void mouseMove(Element &e, const ScreenPoint &mousePos);
  static void scrollUpRaw(Element &e);
  static void scrollDownRaw(Element &e);
  static void scrollUp(Element &e, const ScreenPoint &mousePos) {
    scrollUpRaw(e);
  }
  static void scrollDown(Element &e, const ScreenPoint &mousePos) {
    scrollDownRaw(e);
  }

 protected:
  Element *_content;  // Holds the children themselves, and moves up and down
                      // to "scroll".

 public:
  void addChild(Element *child) override;
  void clearChildren() override;
  Element *findChild(const std::string id) const override;

  void scrollToTop();
  void scrollToBottom();
  bool isScrolledToBottom() const { return _scrolledToBottom; }
  bool isScrolledPastBottom() const;
  bool isScrollBarVisible() const { return _scrollBar->visible(); }
  px_t scrollPos() const { return _content->rect().y; }
  void scrollPos(px_t newPos);

  void doNotScrollToTopOnClear() { _shouldScrollToTopOnClear = false; }
  px_t contentWidth() const { return _content->width(); }
  px_t contentHeight() const { return _content->height(); }
  void resizeToContent() { height(_content->height()); }

  virtual void refresh() override;
};
