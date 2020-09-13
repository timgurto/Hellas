#pragma once

#include <iostream>
#include <string>

using Hitpoints = unsigned;
using Energy = unsigned;
using Percentage = short;
using ArmourClass = short;  // 10 AC = 1%
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

class BasisPoints : public AliasOfShort {
 public:
  BasisPoints(short v) : AliasOfShort(v) {}
  Percentage asPercentage() const;
  std::string asPercentageString() const;
};
