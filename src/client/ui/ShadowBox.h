#ifndef SHADOW_BOX_H
#define SHADOW_BOX_H

#include "Element.h"

// A rectangle resembling a 3D bevel.
class ShadowBox : public Element {
  bool _reversed;

  virtual void refresh() override;

 public:
  ShadowBox(const ScreenRect &rect, bool reversed = false);

  void setReversed(bool reversed);
};

#endif
