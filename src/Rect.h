// (C) 2015 Tim Gurto

#ifndef RECT_H
#define RECT_H

#include "types.h"

struct Point;
struct SDL_Rect;

struct Rect {
    px_t x, y, w, h;

    Rect(px_t x = 0, px_t y = 0, px_t w = 0, px_t h = 0);
    Rect(double x, double y, double w = 0, double h = 0);
    Rect(double x, double y, px_t w, px_t h);
    Rect(const Point &rhs);

    bool collides(const Rect &rhs) const;
};

Rect operator+(const Rect &lhs, const Rect &rhs);

double distance(const Rect &lhs, const Rect &rhs); // Shortest distance

#endif

