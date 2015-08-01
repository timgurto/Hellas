// (C) 2015 Tim Gurto

#ifndef POINT_H
#define POINT_H

#include <SDL.h>

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

SDL_Rect operator+(const SDL_Rect &lhs, const Point &rhs){
    SDL_Rect r = lhs;
    r.x += static_cast<int>(rhs.x + .5);
    r.y += static_cast<int>(rhs.y + .5);
    return r;
}

#endif
