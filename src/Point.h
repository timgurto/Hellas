// (C) 2015 Tim Gurto

#ifndef POINT_H
#define POINT_H

#include <SDL.h>

#include "util.h"

// Describes a 2D point
struct Point{
    double x;
    double y;

    Point(double xArg = 0, double yArg = 0);

    operator SDL_Rect() const;
    bool operator==(const Point &rhs) const;
    Point &operator+=(const Point &rhs);
    Point &operator-=(const Point &rhs);
    Point operator+(const Point &rhs) const;
    Point operator-(const Point &rhs) const;

private:
    static const double EPSILON;
};

inline SDL_Rect operator+(const SDL_Rect &lhs, const Point &rhs){
    SDL_Rect r = lhs;
    r.x += toInt(rhs.x);
    r.y += toInt(rhs.y);
    return r;
}

inline Point operator-(const Point &lhs, const SDL_Rect &rhs){
    Point p = lhs;
    p.x -= rhs.x;
    p.y -= rhs.y;
    return p;
}

#endif
