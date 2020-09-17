#include "combatTypes.h"

#include <iomanip>

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
  lhs >> rhs._raw;
  rhs.onChanged();
  return lhs;
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

double BasisPoints::addTo(Hitpoints base) const {
  auto multiplier = 1.0 + _raw * 0.0001;
  return multiplier * base;
}

const std::string& BasisPoints::display() const { return _memoisedDisplay; }

std::string BasisPoints::displayShort() const {
  auto oss = std::ostringstream{};
  oss << _raw / 100;
  auto fraction = _raw % 100;
  if (fraction > 0) oss << '.' << _raw % 100;
  oss << '%';
  return oss.str();
}

void BasisPoints::onChanged() {
  auto oss = std::ostringstream{};
  oss << _raw / 100;
  oss << '.';
  oss << std::setw(2) << std::setfill('0') << _raw % 100;
  oss << '%';

  _memoisedDisplay = oss.str();
}

Hitpoints Regen::getNextWholeAmount() const {
  auto withRemainder = _raw + _remainder;
  auto whole = withRemainder / 100;
  _remainder = withRemainder - whole;
  return whole;
}

bool Regen::hasValue() const { return _raw != 0; }
