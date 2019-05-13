#include "Point.h"

#include "util.h"

MapPoint toMapPoint(const ScreenPoint &rhs) {
  return {static_cast<double>(rhs.x), static_cast<double>(rhs.y)};
}

ScreenPoint toScreenPoint(const MapPoint &rhs) {
  return {toInt(rhs.x), toInt(rhs.y)};
}

MapPoint normaliseVector(const MapPoint &v) {
  auto length = sqrt(v.x * v.x + v.y * v.y);
  return {v.x / length, v.y / length};
}
