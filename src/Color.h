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

    static const Color MMO_OUTLINE;
    static const Color MMO_HIGHLIGHT;
    static const Color MMO_L_GREEN;
    static const Color MMO_GREEN;
    static const Color MMO_D_GREEN;
    static const Color MMO_L_BLUE;
    static const Color MMO_BLUE;
    static const Color MMO_D_BLUE;
    static const Color MMO_L_GREY;
    static const Color MMO_GREY;
    static const Color MMO_PURPLE;
    static const Color MMO_RED;
    static const Color MMO_D_SKIN;
    static const Color MMO_SKIN;
    static const Color MMO_L_SKIN;

    static const Color GREY_2;
    static const Color GREY_4;
    static const Color GREY_8;

    //static const Color BLUE_HELL;
    static const Color NO_KEY;

    Color(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
    Color(const SDL_Color &rhs);
    Color(Uint32 rhs);

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