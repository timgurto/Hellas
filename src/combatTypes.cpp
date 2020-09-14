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

double ArmourClass::applyTo(double damage) const {
  if (_raw <= 0) return damage;
  if (_raw >= 1000) return 0;
  auto multiplier = (1000 - _raw) / 1000.0;
  return multiplier * damage;
}

ArmourClass ArmourClass::modifyByLevelDiff(Level attacker, Level target) const {
  auto diff = attacker - target;
  return _raw - diff * 30;
}

double BasisPoints::asChance() const {
  if (_raw < 0) return 0;
  return _raw * 1.0 / 10000;
}

std::string BasisPoints::asPercentageString() const {
  return toString(_raw / 100) + "%";
}
