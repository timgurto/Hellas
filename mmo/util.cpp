#include <cmath>

#include "Point.h"
#include "util.h"

double distance(const Point &a, const Point &b){
    double
        xDelta = a.x - b.x,
        yDelta = a.y - b.y;
    return sqrt(xDelta * xDelta + yDelta * yDelta);
}

Point interpolate(const Point &a, const Point &b, double dist){
    double
        xDelta = b.x - a.x,
        yDelta = b.y - a.y;
    double lengthAB = sqrt(xDelta * xDelta + yDelta * yDelta);

    if (dist >= lengthAB)
        // Target point exceeds b
        return b;

    double
        xNorm = xDelta / lengthAB,
        yNorm = yDelta / lengthAB;
    return Point(a.x + xNorm * dist,
                 a.y + yNorm * dist);
}
