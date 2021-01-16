#pragma once

#include "Rect.h"
#include "types.h"

// Describes a 2D point
template <typename T>
struct Point {
  T x;
  T y;

  Point(T xArg = 0, T yArg = 0) : x(xArg), y(yArg) {}

  Point(const Rect<T> &rect) : x(rect.x), y(rect.y) {}

  bool operator==(const Point &rhs) const {
    return abs(x - rhs.x) <= EPSILON && abs(y - rhs.y) <= EPSILON;
  }

  bool operator!=(const Point &rhs) const { return !(*this == rhs); }

  Point &operator+=(const Point &rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
  }

  Point &operator-=(const Point &rhs) {
    this->x -= rhs.x;
    this->y -= rhs.y;
    return *this;
  }

  Point operator+(const Point &rhs) const {
    Point ret = *this;
    ret += rhs;
    return ret;
  }

  Point operator-(const Point &rhs) const {
    Point ret = *this;
    ret -= rhs;
    return ret;
  }

  Point operator-() const { return {-x, -y}; }

  operator Rect<T>() const { return {x, y, 0, 0}; }

  double length() const { return sqrt(x * x + y * y); };

 private:
  static const double EPSILON;
};

template <typename T>
const double Point<T>::EPSILON = 0.001;

template <typename T>
Rect<T> operator+(const Rect<T> &lhs, const Point<T> &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.w, lhs.h};
}

template <typename T>
Rect<T> operator-(const Rect<T> &lhs, const Point<T> &rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.w, lhs.h};
}

template <typename T>
Point<T> operator-(const Point<T> &lhs, const Rect<T> &rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}

template <typename T>
Point<T> operator+(const Point<T> &lhs, const Rect<T> &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

template <typename T>
Point<T> operator*(const Point<T> &lhs, T rhs) {
  return {lhs.x * rhs, lhs.y * rhs};
}

template <typename T>
Point<T> operator/(const Point<T> &lhs, T rhs) {
  return {lhs.x / rhs, lhs.y / rhs};
}

template <typename T>
bool collision(const Point<T> &point, const Rect<T> &rect) {
  return point.x > rect.x && point.x < rect.x + rect.w && point.y > rect.y &&
         point.y < rect.y + rect.h;
}

template <typename T>
std::ostream &operator<<(std::ostream &lhs, const Point<T> &rhs) {
  lhs << "(" << rhs.x << "," << rhs.y << ")";
  return lhs;
}

using MapPoint = Point<double>;
using ScreenPoint = Point<px_t>;

MapPoint toMapPoint(const ScreenPoint &rhs);
ScreenPoint toScreenPoint(const MapPoint &rhs);

MapPoint normaliseVector(const MapPoint &v);
