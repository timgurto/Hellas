#ifndef POINT_H
#define POINT_H

#include <SDL.h>

// Describes a 2D point
struct Point{
    double x;
    double y;

    Point(double xArg = 0, double yArg = 0);

    operator SDL_Rect() const;
};

#endif
