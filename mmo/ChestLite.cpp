// (C) 2015 Tim Gurto

#include "ChestLite.h"

size_t ChestLite::_currentSerial = 0;

ChestLite::ChestLite(const ChestLite &rhs):
serial(rhs.serial),
location(rhs.location) {}

ChestLite::ChestLite(const Point &loc):
serial(_currentSerial++),
location(loc) {}

ChestLite::ChestLite(size_t s): // For set/map lookup
serial (s),
location(0) {}
