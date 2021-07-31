#pragma once

#include "Label.h"

// A label with
class OutlinedLabel : public Element {
 public:
  OutlinedLabel(const ScreenRect &rect, const std::string &text,
                Element::Justification justificationH = Element::LEFT_JUSTIFIED,
                Element::Justification justificationV = Element::TOP_JUSTIFIED);

  Label *centralLabel() { return _mainText; }

  void setColor(const Color &color);
  void changeText(const std::string &text);

  void refresh() override;

 private:
  Label *_mainText{nullptr};
  std::vector<Label *> _labels;
};
