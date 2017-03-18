#include <iostream>

#include "Point.h"
#include "Rect.h"
#include "types.h"
#include "util.h"

Rect::Rect(px_t xx, px_t yy, px_t ww, px_t hh):
x(xx),
y(yy),
w(ww),
h(hh){}

Rect::Rect(double xx, double yy, double ww, double hh):
x(toInt(xx)),
y(toInt(yy)),
w(toInt(ww)),
h(toInt(hh)){}

Rect::Rect(double xx, double yy, px_t ww, px_t hh):
x(toInt(xx)),
y(toInt(yy)),
w(ww),
h(hh){}

Rect::Rect(const Point &rhs):
x(toInt(rhs.x)),
y(toInt(rhs.y)),
w(0),
h(0){}

bool Rect::collides(const Rect &rhs) const{
    return
        rhs.x < x + w &&
        x < rhs.x + rhs.w &&
        rhs.y < y + h &&
        y < rhs.y + rhs.h;
}

Rect operator+(const Rect &lhs, const Rect &rhs){
    return Rect(lhs.x + rhs.x,
                lhs.y + rhs.y,
                lhs.w + rhs.w,
                lhs.h + rhs.h);
}

double distance(const Rect &lhs, const Rect &rhs){
    const px_t
        aL = lhs.x,
        aR = lhs.x + lhs.w,
        aT = lhs.y,
        aB = lhs.y + lhs.h,
        bL = rhs.x,
        bR = rhs.x + rhs.w,
        bT = rhs.y,
        bB = rhs.y + rhs.h;

    if (aR < bL) // A is to the left of B
        if (aB < bT)
            return distance(Point(aR, aB), Point(bL, bT));
        else if (bB < aT)
            return distance(Point(aR, aT), Point(bL, bB));
        else
            return bL - aR;
    if (bR < aL) // A is to the right of B
        if (aB < bT)
            return distance(Point(aL, aB), Point(bR, bT));
        else if (bB < aT)
            return distance(Point(aL, aT), Point(bR, bB));
        else
            return aL - bR;
    // A and B intersect horizontally
    if (aB < bT)
        return bT - aB;
    if (bB < aT)
        return aT - bB;
    else
        return 0;
}

std::ostream &operator<<(std::ostream &lhs, const Rect &rhs){
    lhs << "x=" << rhs.x
        << " y=" << rhs.y
        << " w=" << rhs.w
        << " h=" << rhs.h;
    return lhs;
}

Rect::operator std::string() const{
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

bool Rect::operator==(const Rect &rhs) const{
    return
        x == rhs.x &&
        y == rhs.y &&
        w == rhs.w &&
        h == rhs.h;
}

bool Rect::operator!=(const Rect &rhs) const{
    return !(*this == rhs);
}
