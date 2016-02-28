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

    static const Color QB_BLACK;
    static const Color QB_BLUE;
    static const Color QB_GREEN;
    static const Color QB_CYAN;
    static const Color QB_RED;
    static const Color QB_MAGENTA;
    static const Color QB_BROWN;
    static const Color QB_WHITE;
    static const Color QB_GREY;
    static const Color QB_LIGHT_BLUE;
    static const Color QB_LIGHT_GREEN;
    static const Color QB_LIGHT_CYAN;
    static const Color QB_LIGHT_RED;
    static const Color QB_LIGHT_MAGENTA;
    static const Color QB_YELLOW;
    static const Color QB_BRIGHT_WHITE;

    static const Color GREY_2;
    static const Color GREY_4;
    static const Color GREY_8;

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

    Uint8 r() const { return _r; }
    Uint8 g() const { return _g; }
    Uint8 b() const { return _b; }

private:
    Uint8 _r, _g, _b;
};

Color operator+(const Color &lhs, const Color &rhs);

#endif