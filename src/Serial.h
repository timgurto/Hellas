#pragma once

#include <iostream>

class Serial {
 public:
  Serial() : _raw(UNINITIALISED) {}
  static Serial Inventory() { return {INVENTORY}; }
  static Serial Gear() { return {GEAR}; }

  static Serial Generate();

  bool operator==(Serial rhs) const { return _raw == rhs._raw; }
  bool operator<(Serial rhs) const { return _raw < rhs._raw; }

  bool isEntity() const { return _raw >= FIRST_ENTITY; }
  bool isInitialised() const { return _raw != UNINITIALISED; }
  bool isInventory() const { return _raw == INVENTORY; }
  bool isGear() const { return _raw == GEAR; }

 private:
  Serial(size_t n) { _raw = n; }
  size_t _raw;

  static const size_t INVENTORY = 0, GEAR = 1, UNINITIALISED = 2,
                      FIRST_ENTITY = 3;

  friend std::istream &operator>>(std::istream &lhs, Serial &rhs);
  friend std::ostream &operator<<(std::ostream &lhs, Serial &rhs);
};
