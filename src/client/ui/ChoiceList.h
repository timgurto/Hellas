#ifndef CHOICE_LIST_H
#define CHOICE_LIST_H

#include <string>

#include "List.h"
#include "ShadowBox.h"

class Client;

// A list of elements with IDs; up to one item can be chosen at a time.
class ChoiceList : public List {
  // Note that these three IDs may be invalidated by clearChildren().  Check
  // with verifyBoxes().
  std::string _selectedID;
  std::string _mouseOverID;
  std::string _mouseDownID;
  ShadowBox *_selectedBox, *_mouseOverBox, *_mouseDownBox;
  Element *_boxLayer;  // shape/position is a copy of List::_content

  const std::string &getIdFromMouse(double mouseY, int *index = nullptr) const;
  bool contentCollision(const ScreenPoint &p) const;

  static void markMouseDown(Element &e, const ScreenPoint &mousePos);
  static void toggle(Element &e, const ScreenPoint &mousePos);
  static void markMouseOver(Element &e, const ScreenPoint &mousePos);

 public:
  ChoiceList(const ScreenRect &rect, px_t childHeight, Client &client);

  virtual void refresh() override;

  const std::string &getSelected() { return _selectedID; }
  void clearSelection();
  void manuallySelect(const std::string &id);

  void verifyBoxes();  // Call after changing the list's contents.

  using onSelect_t = void (*)(Client &client);
  onSelect_t onSelect{nullptr};
};

#endif
