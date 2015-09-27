// (C) 2015 Tim Gurto

#include <cmath>

#include "Point.h"
#include "util.h"

const double Point::EPSILON = 0.001;

Point::Point(double xArg, double yArg):
x(xArg),
y(yArg){}

Point::operator SDL_Rect() const{
    SDL_Rect r;
    r.x = toInt(x);
    r.y = toInt(y);
    r.w = r.h = 0;
    return r;
}

bool Point::operator==(const Point &rhs) const{
    return
        abs(x - rhs.x) <= EPSILON &&
        abs(y - rhs.y) <= EPSILON;
}

Point &Point::operator+=(const Point &rhs){
    *this = *this + rhs;
    return *this;
}

Point &Point::operator-=(const Point &rhs){
    *this = *this - rhs;
    return *this;
}

Point Point::operator+(const Point &rhs) const{
    Point ret = *this;
    ret.x += rhs.x;
    ret.y += rhs.y;
    return ret;
}

Point Point::operator-(const Point &rhs) const{
    Point ret = *this;
    ret.x -= rhs.x;
    ret.y -= rhs.y;
    return ret;
}
