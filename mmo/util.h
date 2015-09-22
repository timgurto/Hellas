// (C) 2015 Tim Gurto

#ifndef UTIL_H
#define UTIL_H

#include <cstdlib>
#include <sstream>

#include "Point.h"

struct Point;

const double SQRT_2 = 1.4142135623731;

bool isDebug();

inline double randDouble(){ return static_cast<double>(rand()) / RAND_MAX; }

double distance(const Point &a, const Point &b);
double distance(const Point &p, const Point &a, const Point &b); // point P to line AB

// Return the point between a and b which is dist from point a,
// or return b if dist exceeds the distance between a and b.
Point interpolate(const Point &a, const Point &b, double dist);

template<typename T1>
std::string makeArgs(T1 val1){
    std::ostringstream oss;
    oss << val1;
    return oss.str();
}

template<typename T1, typename T2>
std::string makeArgs(T1 val1, T2 val2){
    std::ostringstream oss;
    oss << val1 << ',' << val2;
    return oss.str();
}

template<typename T1, typename T2, typename T3>
std::string makeArgs(T1 val1, T2 val2, T3 val3){
    std::ostringstream oss;
    oss << val1 << ',' << val2 << ',' << val3;
    return oss.str();
}

inline SDL_Rect makeRect(int x = 0, int y = 0, int w = 0, int h = 0){
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

inline SDL_Rect makeRect(double x, double y, int w = 0, int h = 0){
    SDL_Rect r;
    r.x = static_cast<int>(x + .5);
    r.y = static_cast<int>(y + .5);
    r.w = w;
    r.h = h;
    return r;
}

inline SDL_Rect makeRect(double x, double y, double w, double h){
    SDL_Rect r;
    r.x = static_cast<int>(x + .5);
    r.y = static_cast<int>(y + .5);
    r.w = static_cast<int>(w + .5);
    r.h = static_cast<int>(h + .5);
    return r;
}

inline SDL_Rect operator+(const SDL_Rect &lhs, const SDL_Rect &rhs){
    SDL_Rect r;
    r.x = lhs.x + rhs.x;
    r.y = lhs.y + rhs.y;
    r.w = lhs.w + rhs.w;
    r.h = lhs.h + rhs.h;
    return r;
}

inline SDL_Rect operator-(const SDL_Rect &lhs, const SDL_Rect &rhs){
    SDL_Rect r;
    r.x = lhs.x - rhs.x;
    r.y = lhs.y - rhs.y;
    r.w = lhs.w - rhs.w;
    r.h = lhs.h - rhs.h;
    return r;
}

inline SDL_Rect makeRect(const Point &p){
    return makeRect(p.x, p.y);
}

bool collision(const Point &point, const SDL_Rect &rect);

template<typename T>
T min(const T &lhs, const T &rhs) {
    return lhs < rhs ? lhs : rhs;
}

template<typename T>
T max(const T &lhs, const T &rhs) {
    return rhs < lhs ? lhs : rhs;
}

#endif
