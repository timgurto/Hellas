#ifndef COLOR_BLOCK_H
#define COLOR_BLOCK_H

#include "../../Color.h"
#include "Element.h"

// A colored, filled rectangle
class ColorBlock : public Element {
 public:
  ColorBlock(const ScreenRect &rect, const Color &color = BACKGROUND_COLOR);

  void changeColor(const Color &newColor);
  const Color &color() const { return _color; }

  virtual void refresh() override;

 private:
  Color _color;
};

#endif
