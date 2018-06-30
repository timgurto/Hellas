#include "Indicator.h"
#include "../Texture.h"

Indicator::Images Indicator::images;
bool Indicator::initialized = false;

Indicator::Indicator(const ScreenPoint& loc, Status initial)
    : Picture(loc.x, loc.y, {}) {
  if (!initialized) initialize();
  set(initial);
}

void Indicator::set(Status status) { changeTexture(images[status]); }

void Indicator::initialize() {
  images[SUCCEEDED] = {"Images/UI/indicator-success.png", Color::MAGENTA};
  images[FAILED] = {"Images/UI/indicator-failure.png", Color::MAGENTA};
  images[IN_PROGRESS] = {"Images/UI/indicator-progress.png", Color::MAGENTA};
  initialized = true;
}
