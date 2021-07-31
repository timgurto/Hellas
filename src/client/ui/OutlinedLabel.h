#pragma once

#include "Label.h"

// A label with
class OutlinedLabel : public Element {
 public:
  OutlinedLabel(const ScreenRect &rect, const std::string &text,
                Element::Justification justificationH = Element::LEFT_JUSTIFIED,
                Element::Justification justificationV = Element::TOP_JUSTIFIED);

  Label *centralLabel() { return _central; }

  void setColor(const Color &color);
  void changeText(const std::string &text);

  void refresh() override;

 private:
  /*
  hue
  lCr
  gdf
  */
  Label *_central{nullptr};
  Label *_u{nullptr};
  Label *_d{nullptr};
  Label *_l{nullptr};
  Label *_r{nullptr};
  Label *_e{nullptr};
  Label *_f{nullptr};
  Label *_g{nullptr};
  Label *_h{nullptr};
};
