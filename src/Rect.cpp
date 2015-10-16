// (C) 2015 Tim Gurto

#include "Point.h"
#include "Rect.h"
#include "util.h"

Rect::Rect(int xx, int yy, int ww, int hh):
x(xx),
y(yy),
w(ww),
h(hh){}

Rect::Rect(const Point &rhs):
x(toInt(rhs.x)),
y(toInt(rhs.y)),
w(0),
h(0){}

Rect operator+(const Rect &lhs, const Rect &rhs){
    return Rect(lhs.x + rhs.x,
                lhs.y + rhs.y,
                lhs.w + rhs.w,
                lhs.h + rhs.h);
}

