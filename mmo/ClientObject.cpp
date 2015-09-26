// (C) 2015 Tim Gurto

#include "ClientObject.h"
#include "Client.h"
#include "Color.h"
#include "Server.h"
#include "util.h"

ClientObject::ClientObject(const ClientObject &rhs):
Entity(rhs),
_serial(rhs._serial){}

ClientObject::ClientObject(size_t serialArg, const EntityType &type, const Point &loc):
Entity(type, loc),
_serial(serialArg){}

void ClientObject::onLeftClick(Client &client) const{
    if (type().canGather()) {
        std::ostringstream oss;
        oss << '[' << CL_GATHER << ',' << _serial << ']';
        client.socket().sendMessage(oss.str());
        client.prepareAction(std::string("Gathering") + type().name());
    }
}

std::vector<std::string> ClientObject::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back(type().name());
    if (type().canGather() && distance(location(),
                                       client.character().location()) > Server::ACTION_DISTANCE)
        text.push_back("Out of range");
    return text;
}
