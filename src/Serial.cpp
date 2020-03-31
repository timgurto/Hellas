#include "Serial.h"

std::istream &operator>>(std::istream &lhs, Serial &rhs) {
  return lhs >> rhs._raw;
}
std::ostream &operator<<(std::ostream &lhs, Serial &rhs) {
  return lhs << rhs._raw;
}

Serial Serial::Generate() {
  static auto nextToAllocate = FIRST_ENTITY;
  return {nextToAllocate++};
}
