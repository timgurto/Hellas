// (C) 2015 Tim Gurto

#include <cmath>

#include "Point.h"
#include "util.h"

const double Point::EPSILON = 0.001;

Point::Point(double xArg, double yArg):
x(xArg),
y(yArg){}

Point::Point(const Rect &rect):
x(rect.x),
y(rect.y){}

bool Point::operator==(const Point &rhs) const{
    return
        abs(x - rhs.x) <= EPSILON &&
        abs(y - rhs.y) <= EPSILON;
}

Point &Point::operator+=(const Point &rhs){
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
}

Point &Point::operator-=(const Point &rhs){
    this->x -= rhs.x;
    this->y -= rhs.y;
    return *this;
}

Point Point::operator+(const Point &rhs) const{
    Point ret = *this;
    ret += rhs;
    return ret;
}

Point Point::operator-(const Point &rhs) const{
    Point ret = *this;
    ret -= rhs;
    return ret;
}
