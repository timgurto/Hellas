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

    Color(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
    Color(const SDL_Color &rhs);

    operator SDL_Color ()const;
    operator Uint32() const; // Used by some SDL functions
    Color operator/(double d) const;
    Color operator*(double d) const;

private:
    Uint8 _r, _g, _b;
};

#endif