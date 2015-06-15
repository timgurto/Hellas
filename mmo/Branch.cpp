#include "Branch.h"
#include "Color.h"
#include "util.h"

int Branch::_currentSerial = 0;
EntityType Branch::_entityType(makeRect(-10, -5));


Branch::Branch(const Branch &rhs):
_serial(rhs._serial),
_entity(rhs._entity){}

Branch::Branch(const Point &loc):
_serial(_currentSerial++),
_entity(_entityType, loc){}

Branch::Branch(int serialArg, const Point &loc):
_serial(serialArg),
_entity(_entityType, loc){}

bool Branch::operator<(const Branch &rhs) const{
    return _serial < rhs._serial;
}

bool Branch::operator==(const Branch &rhs) const{
    return _serial == rhs._serial;
}

int Branch::serial() const{
    return _serial;
}

const Point &Branch::location() const{
    return _entity.location();
}

const Entity &Branch::entity() const{
    return _entity;
}

void Branch::image(const std::string &filename){
    _entityType.image(filename);
}

