// (C) 2015 Tim Gurto

#include <cassert>
#include <cmath>

#include "Args.h"
#include "Point.h"
#include "util.h"

extern Args cmdLineArgs;

bool isDebug(){
    static bool debug = cmdLineArgs.contains("debug");
    return debug;
}

double distance(const Point &a, const Point &b){
    double
        xDelta = a.x - b.x,
        yDelta = a.y - b.y;
    return sqrt(xDelta * xDelta + yDelta * yDelta);
}

double distance(const Point &p, const Point &a, const Point &b){
    double numerator = (b.y - a.y) * p.x -
                       (b.x - a.x) * p.y +
                       b.x * a.y -
                       b.y * a.x;
    numerator = abs(numerator);
    double denominator = (b.y - a.y) * (b.y - a.y) +
                         (b.x - a.x) * (b.x - a.x);
    assert (denominator > 0);
    denominator = sqrt(denominator);
    return numerator / denominator;
}

Point interpolate(const Point &a, const Point &b, double dist){
    double
        xDelta = b.x - a.x,
        yDelta = b.y - a.y;
    if (xDelta == 0 && yDelta == 0)
        return a;
    double lengthAB = sqrt(xDelta * xDelta + yDelta * yDelta);
    assert (lengthAB > 0);

    if (dist >= lengthAB)
        // Target point exceeds b
        return b;

    double
        xNorm = xDelta / lengthAB,
        yNorm = yDelta / lengthAB;
    return Point(a.x + xNorm * dist,
                 a.y + yNorm * dist);
}

bool collision(const Point &point, const SDL_Rect &rect){
    return
        point.x > rect.x &&
        point.y > rect.y &&
        point.x < rect.x + rect.w &&
        point.y < rect.y + rect.h;
}
