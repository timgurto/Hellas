#pragma once

#include <iostream>
#include <string>

using Level = short;

using Hitpoints = unsigned;
using Energy = unsigned;
using Percentage = short;
using BonusDamage = int;
using Regen = int;

class AliasOfShort {
 public:
  AliasOfShort(short v) : _raw(v) {}

  bool operator==(const AliasOfShort& rhs) const;
  bool operator!=(const AliasOfShort& rhs) const;
  void operator+=(const AliasOfShort& rhs);
  operator bool() const;

 protected:
  short _raw;

  friend std::ostream& operator<<(std::ostream& lhs, const AliasOfShort& rhs);
  friend std::istream& operator>>(std::istream& lhs, AliasOfShort& rhs);
};

std::ostream& operator<<(std::ostream& lhs, const AliasOfShort& rhs);
std::istream& operator>>(std::istream& lhs, AliasOfShort& rhs);

// 10 AC = 1%
class ArmourClass : public AliasOfShort {
 public:
  ArmourClass(short v) : AliasOfShort(v) {}
  double applyTo(double damage) const;
  ArmourClass modifyByLevelDiff(Level attacker, Level target) const;
};

// 100 points = 1.00%
class BasisPoints : public AliasOfShort {
 public:
  BasisPoints(short v) : AliasOfShort(v) {}
  double asChance() const;
  std::string asPercentageString() const;
};
