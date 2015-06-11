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

private:
    static const double EPSILON;
};

inline SDL_Rect operator+(const SDL_Rect &lhs, const Point &rhs){
    SDL_Rect r = lhs;
    r.x += static_cast<int>(rhs.x);
    r.y += static_cast<int>(rhs.y);
    return r;
}

#endif
