// (C) 2015 Tim Gurto

#include "Chest.h"
#include "ChestLite.h"
#include "Client.h"
#include "Color.h"
#include "Server.h"
#include "util.h"

EntityType Chest::_entityType(makeRect(-14, -12));

Chest::Chest(const Chest &rhs):
Entity(rhs),
_serial(rhs._serial){}

Chest::Chest(size_t serialArg, const Point &loc):
Entity(_entityType, loc),
_serial(serialArg){}

void Chest::onLeftClick(Client &client) const{
    
}

std::vector<std::string> Chest::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back("Chest");
    if (distance(location(), client.character().location()) > Server::ACTION_DISTANCE)
        text.push_back("Out of range");
    return text;
}
