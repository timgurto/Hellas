#ifndef COLOR_H
#define COLOR_H

#include <iostream>

#include "SDL.h"

class Color {
 public:
  static const Color TODO;
  static const Color BLACK;
  static const Color BLUE_HELL;
  static const Color NO_KEY;

  static const Color ERR;

  Color(Uint8 r, Uint8 g, Uint8 b);
  Color(const SDL_Color &rhs);
  Color(Uint32 rhs);

  operator SDL_Color() const;
  operator Uint32() const;  // Used by some SDL functions

  // Multiply or divide color by a scalar
  Color operator/(double s) const;
  Color operator/(int s) const;  // to avoid ambiguity with implicit Uint32 cast
  Color operator*(double s) const;
  Color operator*(int s) const;  // to avoid ambiguity with implicit Uint32 cast

  Uint8 r() const { return _r; }
  Uint8 g() const { return _g; }
  Uint8 b() const { return _b; }

 private:
  Uint8 _r, _g, _b;
};

Color operator+(const Color &lhs, const Color &rhs);

std::ostream &operator<<(std::ostream &lhs, const Color &rhs);

#endif
