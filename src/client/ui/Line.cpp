#include "Line.h"
#include "../Renderer.h"

extern Renderer renderer;

Line::Line(ScreenPoint topLeft, px_t length, Orientation orientation)
    : Element({topLeft.x, topLeft.y, 2, 2}), _orientation(orientation) {
  if (_orientation == HORIZONTAL)
    width(length);
  else
    height(length);
}

void Line::refresh() {
  const auto darkRect = _orientation == HORIZONTAL
                            ? ScreenRect{0, 0, rect().w, 1}
                            : ScreenRect{0, 0, 1, rect().h};
  renderer.setDrawColor(SHADOW_DARK);
  renderer.fillRect(darkRect);

  const auto lightRect = _orientation == HORIZONTAL
                             ? ScreenRect{0, 1, rect().w, 1}
                             : ScreenRect{1, 0, 1, rect().h};

  renderer.setDrawColor(SHADOW_LIGHT);
  renderer.fillRect(lightRect);
}
