// (C) 2015 Tim Gurto

#ifndef COLOR_H
#define COLOR_H

#include "SDL.h"

class Color{
public:
    static const Color BLACK;
    static const Color BLUE;
    static const Color GREEN;
    static const Color CYAN;
    static const Color RED;
    static const Color MAGENTA;
    static const Color YELLOW;
    static const Color WHITE;

    static const Color BLUE_HELL;
    static const Color NO_KEY;

    Color(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
    Color(const SDL_Color &rhs);

    operator SDL_Color() const;
    operator Uint32() const; // Used by some SDL functions

    // Multiply or divide color by a scalar
    Color operator/(double s) const;
    Color operator/(int s) const; // to avoid ambiguity with implicit Uint32 cast
    Color operator*(double s) const;
    Color operator*(int s) const; // to avoid ambiguity with implicit Uint32 cast

    inline Uint8 r() const { return _r; }
    inline Uint8 g() const { return _g; }
    inline Uint8 b() const { return _b; }

private:
    Uint8 _r, _g, _b;
};

Color operator+(const Color &lhs, const Color &rhs);

#endif