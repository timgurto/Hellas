#pragma once

#include <iostream>
#include <string>

using Level = short;

using Hitpoints = unsigned;
using Energy = unsigned;
using Percentage = short;

class AliasOfShort {
 public:
  AliasOfShort(short v) : _raw(v) {}

  bool operator==(const AliasOfShort& rhs) const;
  bool operator!=(const AliasOfShort& rhs) const;
  void operator+=(const AliasOfShort& rhs);
  void operator*=(int scalar);
  operator bool() const;

 protected:
  virtual void onChanged() {}

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
  BasisPoints(short v) : AliasOfShort(v) { onChanged(); }
  double asChance(bool allowNegative = false) const;
  double addTo(double base) const;
  const std::string& display() const;
  std::string displayShort() const;

 private:
  virtual void onChanged() override;
  std::string _memoisedDisplay;
};

// 100 points = 1/s
class Regen : public AliasOfShort {
 public:
  Regen(short v) : AliasOfShort(v) {}
  Hitpoints getNextWholeAmount() const;
  bool hasValue() const;
  std::string displayShort() const;

 private:
  mutable short _remainder{0};
};

class Hundredths : public AliasOfShort {
 public:
  Hundredths(short v) : AliasOfShort(v) { onChanged(); }
  Hitpoints effectiveValue() const;
  const std::string& display() const;

 private:
  virtual void onChanged() override;
  std::string _memoisedDisplay;
};
