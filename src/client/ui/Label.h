#ifndef LABEL_H
#define LABEL_H

#include <string>

#include "Element.h"

class Color;

// Displays text.
class Label : public Element {
  Justification _justificationH, _justificationV;
  bool _matchWidth;
  Color _color;

 protected:
  std::string _text;

 public:
  Label(const ScreenRect &rect, const std::string &text,
        Justification justificationH = LEFT_JUSTIFIED,
        Justification justificationV = TOP_JUSTIFIED);

  virtual void refresh() override;

  void setColor(const Color &color);
  void setJustificationH(const Justification justification);

  void
  matchW();  // Set the label's width to the width of the contained text image.

  void changeText(const std::string &text);
};

#endif
