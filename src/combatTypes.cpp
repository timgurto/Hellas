#include "combatTypes.h"

#include "util.h"

bool BasisPoints::operator==(const BasisPoints& rhs) const {
  return _raw == rhs._raw;
}

bool BasisPoints::operator!=(const BasisPoints& rhs) const {
  return !(*this == rhs);
}

void BasisPoints::operator+=(const BasisPoints& rhs) { _raw += rhs._raw; }

BasisPoints::operator bool() const { return _raw > 0; }

Percentage BasisPoints::asPercentage() const {
  if (_raw < 0) return 0;
  return _raw / 100;
}

std::string BasisPoints::asPercentageString() const {
  return toString(asPercentage()) + "%";
}

std::ostream& operator<<(std::ostream& lhs, const BasisPoints& rhs) {
  return lhs << rhs._raw;
}

std::istream& operator>>(std::istream& lhs, BasisPoints& rhs) {
  return lhs >> rhs._raw;
}
