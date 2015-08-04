// (C) 2015 Tim Gurto

#include "Branch.h"
#include "BranchLite.h"
#include "Client.h"
#include "Color.h"
#include "Server.h"
#include "util.h"

EntityType Branch::_entityType(makeRect(-14, -12));

Branch::Branch(const Branch &rhs):
Entity(rhs),
_serial(rhs._serial){}

Branch::Branch(size_t serialArg, const Point &loc):
Entity(_entityType, loc),
_serial(serialArg){}

void Branch::onLeftClick(Client &client) const{
    std::ostringstream oss;
    oss << '[' << CL_COLLECT_BRANCH << ',' << _serial << ']';
    client.socket().sendMessage(oss.str());
    client.prepareAction("Gathering branch");
}

std::vector<std::string> Branch::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back("Branch");
    if (distance(location(), client.character().location()) > Server::ACTION_DISTANCE)
        text.push_back("Out of range");
    return text;
}
