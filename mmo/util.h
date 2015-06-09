#ifndef UTIL_H
#define UTIL_H

#include <sstream>

struct Point;

const double SQRT_2 = 1.4142135623731;

double distance(const Point &a, const Point &b);

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

inline SDL_Rect makeRect(int x, int y, int w, int h){
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

inline SDL_Rect makeRect(double x, double y, int w, int h){
    SDL_Rect r;
    r.x = static_cast<int>(x + .5);
    r.y = static_cast<int>(y + .5);
    r.w = w;
    r.h = h;
    return r;
}

bool collision(const Point &point, const SDL_Rect &rect);

#endif
