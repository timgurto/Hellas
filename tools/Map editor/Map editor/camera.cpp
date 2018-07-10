#include "../../../src/client/Renderer.h"

#include "Map.h"
#include "main.h"

extern std::pair<int, int> offset;
extern Map map;
extern Renderer renderer;
extern int zoomLevel;

void pan(Direction dir) {
  const auto PAN_AMOUNT = 200;
  switch (dir) {
    case UP:
      offset.second -= zoomed(PAN_AMOUNT);
      break;
    case DOWN:
      offset.second += zoomed(PAN_AMOUNT);
      break;
    case LEFT:
      offset.first -= zoomed(PAN_AMOUNT);
      break;
    case RIGHT:
      offset.first += zoomed(PAN_AMOUNT);
      break;
  }
}

void enforcePanLimits() {
  if (offset.first < 0)
    offset.first = 0;
  else {
    const auto maxXOffset =
        static_cast<int>(map.width()) - zoomed(renderer.width());
    if (offset.first > maxXOffset) offset.first = maxXOffset;
  }
  if (offset.second < 0)
    offset.second = 0;
  else {
    const auto maxYOffset =
        static_cast<int>(map.height()) - zoomed(renderer.height());
    if (offset.second > maxYOffset) offset.second = maxYOffset;
  }
}

void zoomIn() {
  auto oldWidth = zoomed(renderer.width());
  auto oldHeight = zoomed(renderer.height());
  ++zoomLevel;
  auto newWidth = zoomed(renderer.width());
  auto newHeight = zoomed(renderer.height());
  offset.first += (oldWidth - newWidth) / 2;
  offset.second += (oldHeight - newHeight) / 2;
}

void zoomOut() {
  auto oldWidth = zoomed(renderer.width());
  auto oldHeight = zoomed(renderer.height());
  --zoomLevel;
  auto newWidth = zoomed(renderer.width());
  auto newHeight = zoomed(renderer.height());
  offset.first -= (newWidth - oldWidth) / 2;
  offset.second -= (newHeight - oldHeight) / 2;
}

int zoomed(int value) {
  if (zoomLevel > 0)
    return value >> zoomLevel;
  else if (zoomLevel < 0)
    return value << -zoomLevel;
  return value;
}

int unzoomed(int value) {
  if (zoomLevel < 0)
    return value >> -zoomLevel;
  else if (zoomLevel > 0)
    return value << zoomLevel;
  return value;
}

double zoomed(double value) {
  if (zoomLevel > 0) {
    for (auto i = 0; i != zoomLevel; ++i) value /= 2.0;
    return value;
  } else if (zoomLevel < 0) {
    for (auto i = 0; i != -zoomLevel; ++i) value *= 2.0;
    return value;
  }
  return value;
}

double unzoomed(double value) {
  if (zoomLevel > 0) {
    for (auto i = 0; i != zoomLevel; ++i) value *= 2.0;
    return value;
  } else if (zoomLevel < 0) {
    for (auto i = 0; i != -zoomLevel; ++i) value /= 2.0;
    return value;
  }
  return value;
}
