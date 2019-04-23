#include "Podes.h"

const Podes Podes::MELEE_RANGE{4};

std::istream &operator>>(std::istream &lhs, Podes &rhs) {
  lhs >> rhs._p;
  return lhs;
}

std::ostream &operator<<(std::ostream &lhs, Podes rhs) {
  lhs << rhs._p;
  return lhs;
}

Podes Podes::FromPixels(px_t p) { return toInt(p / 7); }

std::string Podes::displayFromPixels(double d) {
  auto raw = d / 7.0;
  auto oss = std::ostringstream{};
  oss.precision(3);
  oss << raw;
  return oss.str();
}
