#include <cmath>

#include "Point.h"

const double Point::EPSILON = 0.001;

Point::Point(double xArg, double yArg):
x(xArg),
y(yArg){}

Point::operator SDL_Rect() const{
    SDL_Rect r;
    r.x = static_cast<int>(x + .5);
    r.y = static_cast<int>(y + .5);
    r.w = r.h = 0;
    return r;
}

bool Point::operator==(const Point &rhs) const{
    return
        abs(x - rhs.x) <= EPSILON &&
        abs(y - rhs.y) <= EPSILON;
}
