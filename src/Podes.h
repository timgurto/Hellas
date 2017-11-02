#pragma once

#include <iostream>

#include "util.h"

class Podes { // 1 Pous = 7 pixels = 1 foot
public:
    Podes(int p = 0) :_p(p) {}
    px_t toPixels() const { return _p * 7; }

    static const Podes MELEE_RANGE;

private:
    int _p;
    friend std::ostream &operator << (std::ostream &lhs, Podes rhs);
    friend std::istream &operator >> (std::istream &lhs, Podes &rhs);
};
