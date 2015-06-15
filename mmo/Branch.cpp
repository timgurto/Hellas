#include "Branch.h"
#include "Color.h"
#include "util.h"

EntityType Branch::_entityType(makeRect(-10, -5));


Branch::Branch(const Branch &rhs):
_serial(rhs._serial),
_entity(rhs._entity){}

Branch::Branch(size_t serialArg, const Point &loc):
_serial(serialArg),
_entity(_entityType, loc){}
