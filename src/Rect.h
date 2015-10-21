// (C) 2015 Tim Gurto

#ifndef RECT_H
#define RECT_H

struct Point;
struct SDL_Rect;

struct Rect {
    int x, y, w, h;

    Rect(int x = 0, int y = 0, int w = 0, int h = 0);
    Rect(double x, double y, double w = 0, double h = 0);
    Rect(double x, double y, int w, int h);
    Rect(const Point &rhs);

    bool collides(const Rect &rhs) const;
};

Rect operator+(const Rect &lhs, const Rect &rhs);

#endif

