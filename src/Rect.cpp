#include "Rect.h"

#include "util.h"

ScreenRect toScreenRect(const MapRect &rhs) {
  return {toInt(rhs.x), toInt(rhs.y), toInt(rhs.w), toInt(rhs.h)};
}

MapRect toMapRect(const ScreenRect &rhs) {
  return {static_cast<double>(rhs.x), static_cast<double>(rhs.y),
          static_cast<double>(rhs.w), static_cast<double>(rhs.h)};
}
