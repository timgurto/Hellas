#include "combatTypes.h"

#include "util.h"

bool AliasOfShort::operator==(const AliasOfShort& rhs) const {
  return _raw == rhs._raw;
}

bool AliasOfShort::operator!=(const AliasOfShort& rhs) const {
  return !(*this == rhs);
}

void AliasOfShort::operator+=(const AliasOfShort& rhs) { _raw += rhs._raw; }

AliasOfShort::operator bool() const { return _raw > 0; }

std::ostream& operator<<(std::ostream& lhs, const AliasOfShort& rhs) {
  return lhs << rhs._raw;
}

std::istream& operator>>(std::istream& lhs, AliasOfShort& rhs) {
  return lhs >> rhs._raw;
}

Percentage BasisPoints::asPercentage() const {
  if (_raw < 0) return 0;
  return _raw / 100;
}

std::string BasisPoints::asPercentageString() const {
  return toString(asPercentage()) + "%";
}
