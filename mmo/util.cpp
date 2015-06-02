#include <cmath>

#include "Point.h"
#include "util.h"

double distance(const Point &a, const Point &b){
    double
        xDelta = a.x - b.x,
        yDelta = a.y - b.y;
    return sqrt(xDelta * xDelta + yDelta * yDelta);
}
