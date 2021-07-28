#ifndef LINE_H
#define LINE_H

#include "Element.h"

// A straight horizontal or vertical line.
class Line : public Element {
  Orientation _orientation;

  virtual void refresh() override;

 public:
  // TODO: Take a ScreenPoint instead of x and y
  Line(ScreenPoint topLeft, px_t length,
       Orientation _orientation = HORIZONTAL);  // x,y = top-left corner.
};

#endif
