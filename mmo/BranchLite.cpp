#include "BranchLite.h"

size_t BranchLite::_currentSerial = 0;

const Uint32 BranchLite::ACTION_TIME = 1000;

BranchLite::BranchLite(const BranchLite &rhs):
serial(rhs.serial),
location(rhs.location) {}

BranchLite::BranchLite(const Point &loc):
serial(_currentSerial++),
location(loc) {}

BranchLite::BranchLite(size_t s): // For set/map lookup
serial (s),
location(0) {}
