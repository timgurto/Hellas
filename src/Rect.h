#ifndef RECT_H
#define RECT_H

#include <string>

#include "types.h"

struct SDL_Rect;

template <typename T>
struct Rect {
  T x, y, w, h;

  Rect(T xx = 0, T yy = 0, T ww = 0, T hh = 0) : x(xx), y(yy), w(ww), h(hh) {}

  bool overlaps(const Rect &rhs) const {
    return rhs.x <= x + w && x <= rhs.x + rhs.w && rhs.y <= y + h &&
           y <= rhs.y + rhs.h;
  }

  operator std::string() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
  }

  bool operator==(const Rect &rhs) const {
    return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
  }

  bool operator!=(const Rect &rhs) const { return !(*this == rhs); }

  void operator+=(const Rect &rhs) { *this = *this + rhs; }
};

template <typename T>
Rect<T> operator+(const Rect<T> &lhs, const Rect<T> &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.w + rhs.w, lhs.h + rhs.h};
}

template <typename T>
std::ostream &operator<<(std::ostream &lhs, const Rect<T> &rhs) {
  lhs << "x=" << rhs.x << " y=" << rhs.y << " w=" << rhs.w << " h=" << rhs.h;
  return lhs;
}

template <typename T>
double distance(
    const Rect<T> &lhs,
    const Rect<T> &rhs) {  // Shortest distance between two rectangles
  const T aL = lhs.x, aR = lhs.x + lhs.w, aT = lhs.y, aB = lhs.y + lhs.h,
          bL = rhs.x, bR = rhs.x + rhs.w, bT = rhs.y, bB = rhs.y + rhs.h;

  if (aR < bL)  // A is to the left of B
    if (aB < bT)
      return distance({aR, aB}, {bL, bT});
    else if (bB < aT)
      return distance({aR, aT}, {bL, bB});
    else
      return bL - aR;
  if (bR < aL)  // A is to the right of B
    if (aB < bT)
      return distance({aL, aB}, {bR, bT});
    else if (bB < aT)
      return distance({aL, aT}, {bR, bB});
    else
      return aL - bR;
  // A and B intersect horizontally
  if (aB < bT) return bT - aB;
  if (bB < aT)
    return aT - bB;
  else
    return 0;
}

using MapRect = Rect<double>;
using ScreenRect = Rect<px_t>;

ScreenRect toScreenRect(const MapRect &rhs);

#endif
