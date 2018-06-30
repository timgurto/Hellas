#ifndef LIST_H
#define LIST_H

#include "Element.h"
#include "Picture.h"

// A scrollable vertical list of elements of uniform height
class List : public Element {
 public:
  static const px_t ARROW_W, ARROW_H, CURSOR_HEIGHT, SCROLL_AMOUNT;

 protected:
  static void mouseUp(Element &e, const ScreenPoint &mousePos);

 private:
  bool _mouseDownOnCursor;
  px_t _cursorOffset;  // y-offset of the mouse on the cursor.

  px_t _childHeight;

  Element *_scrollBar;
  static Texture  // Arrow images
      whiteUp,
      whiteDown, greyUp, greyDown;
  Picture *_whiteUp{nullptr}, *_whiteDown{nullptr}, *_greyUp{nullptr},
      *_greyDown{nullptr};
  Element *_cursor{
      nullptr};  // Shows position on the scroll bar, and can be dragged.
  void updateScrollBar();  // Update the appearance of the scroll bar.
  bool _scrolledToBottom;

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
  Element *_content;  // Holds the list items themselves, and moves up and down
                      // to "scroll".

 public:
  List(const ScreenRect &rect, px_t childHeight = Element::TEXT_HEIGHT);

  void addChild(Element *child) override;
  void clearChildren() override;
  Element *findChild(const std::string id) const override;

  void scrollToBottom();
  bool isScrolledToBottom() const { return _scrolledToBottom; }
  bool isScrollBarVisible() const { return _scrollBar->visible(); }
  px_t scrollPos() const { return _content->rect().y; }
  void scrollPos(px_t newPos);
  px_t childHeight() const { return _childHeight; }
  void doNotScrollToTopOnClear() { _shouldScrollToTopOnClear = false; }
  px_t contentWidth() const { return _content->width(); }
  void resizeToContent() { height(_content->height()); }

  size_t size() const { return _content->children().size(); }
  bool empty() const { return size() == 0; }

  virtual void refresh() override;
};

#endif
