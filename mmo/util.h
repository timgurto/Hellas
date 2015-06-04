#ifndef UTIL_H
#define UTIL_H

struct Point;

const double SQRT_2 = 1.4142135623731;

double distance(const Point &a, const Point &b);

// Return the point between a and b which is dist from point a,
// or return b if dist exceeds the distance between a and b.
Point interpolate(const Point &a, const Point &b, double dist);

#endif
