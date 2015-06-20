#include "Branch.h"
#include "Client.h"
#include "Color.h"
#include "util.h"

EntityType Branch::_entityType(makeRect(-10, -5));

Branch::Branch(const Branch &rhs):
Entity(rhs),
_serial(rhs._serial){}

Branch::Branch(size_t serialArg, const Point &loc):
Entity(_entityType, loc),
_serial(serialArg){}

void Branch::onLeftClick(const Client &client){
    std::ostringstream oss;
    oss << '[' << CL_COLLECT_BRANCH << ',' << _serial << ']';
    client.socket().sendMessage(oss.str());
}
