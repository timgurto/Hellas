#include "Podes.h"

std::istream &operator >> (std::istream &lhs, Podes &rhs) {
    lhs >> rhs._p; return lhs;
}

std::ostream &operator << (std::ostream &lhs, Podes rhs) {
    lhs << rhs._p; return lhs;
}