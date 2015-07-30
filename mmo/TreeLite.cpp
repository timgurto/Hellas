#include "TreeLite.h"

size_t TreeLite::_currentSerial = 0;

const Uint32 TreeLite::ACTION_TIME = 6000;

TreeLite::TreeLite(const TreeLite &rhs):
serial(rhs.serial),
location(rhs.location),
_wood(rhs._wood) {}

TreeLite::TreeLite(const Point &loc, size_t wood):
serial(_currentSerial++),
location(loc),
_wood(wood){}

TreeLite::TreeLite(size_t s): // For set/map lookup
serial (s),
location(0) {}

void TreeLite::decrementWood(){
    --_wood;
}
