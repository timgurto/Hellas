#pragma once

#include <iostream>
#include <string>

using Hitpoints = unsigned;
using Energy = unsigned;
using Percentage = short;
using ArmourClass = short;  // 10 AC = 1%
using BonusDamage = int;
using Regen = int;

class BasisPoints {
 public:
  BasisPoints(short v) : _raw(v) {}

  bool operator==(const BasisPoints& rhs) const;
  bool operator!=(const BasisPoints& rhs) const;
  void operator+=(const BasisPoints& rhs);
  operator bool() const;

  Percentage asPercentage() const;
  std::string asPercentageString() const;

 private:
  short _raw;

  friend std::ostream& operator<<(std::ostream& lhs, const BasisPoints& rhs);
  friend std::istream& operator>>(std::istream& lhs, BasisPoints& rhs);
};

std::ostream& operator<<(std::ostream& lhs, const BasisPoints& rhs);
std::istream& operator>>(std::istream& lhs, BasisPoints& rhs);
