#include "TreeLite.h"

size_t TreeLite::_currentSerial = 0;

TreeLite::TreeLite(const TreeLite &rhs):
serial(rhs.serial),
location(rhs.location) {}

TreeLite::TreeLite(const Point &loc):
serial(_currentSerial++),
location(loc) {}

TreeLite::TreeLite(size_t s) // For set/map lookup
:serial (s),
location(0) {}
