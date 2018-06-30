#ifndef CHECK_BOX_H
#define CHECK_BOX_H

#include <string>

#include "Element.h"

// A button which can be clicked, showing visible feedback and performing a
// function.
class CheckBox : public Element {
 private:
  static const px_t Y_OFFSET;  // Shifts the box vertically

  bool &_linkedBool;  // A boolean whose value is tied to this check box
  // _linkedBool's value when last checked.  Used to determine whether a refresh
  // is necessary.
  bool _lastCheckedValue;
  void toggleBool();

  bool _inverse;  // Whether this checkbox should inversely reflect its linked
                  // bool.

  virtual void checkIfChanged() override;

  bool _mouseButtonDown;
  bool _depressed;

  void depress();
  void release(
      bool click);  // click: whether, on release, the check box will toggle

  static void mouseDown(Element &e, const ScreenPoint &mousePos);
  static void mouseUp(Element &e, const ScreenPoint &mousePos);
  static void mouseMove(Element &e, const ScreenPoint &mousePos);

  virtual void refresh() override;

 public:
  static const px_t BOX_SIZE,
      GAP;  // The gap between box and label, if any.

  CheckBox(const ScreenRect &rect, bool &linkedBool,
           const std::string &caption = "", bool inverse = false);
};

#endif
