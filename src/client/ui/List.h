#ifndef LIST_H
#define LIST_H

#include "Scrollable.h"

// A scrollable vertical list of elements of uniform height
class List : public Scrollable {
 private:
  px_t _childHeight;

 public:
  List(const ScreenRect &rect, px_t childHeight = Element::TEXT_HEIGHT);
  px_t childHeight() const { return _childHeight; }

  size_t size() const { return _content->children().size(); }
  bool empty() const { return size() == 0; }

  void addChild(Element *child) override;
};

#endif
