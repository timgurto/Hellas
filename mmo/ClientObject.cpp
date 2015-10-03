// (C) 2015 Tim Gurto

#include "ClientObject.h"
#include "Client.h"
#include "Color.h"
#include "Server.h"
#include "util.h"

ClientObject::ClientObject(const ClientObject &rhs):
Entity(rhs),
_serial(rhs._serial){}

ClientObject::ClientObject(size_t serialArg, const EntityType *type, const Point &loc):
Entity(type, loc),
_serial(serialArg){}

void ClientObject::onLeftClick(Client &client) const{
    if (objectType()->canGather()) {
        std::ostringstream oss;
        oss << '[' << CL_GATHER << ',' << _serial << ']';
        client.socket().sendMessage(oss.str());
        client.prepareAction(std::string("Gathering") + objectType()->name());
        playGatherSound();
    }
}

std::vector<std::string> ClientObject::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back(objectType()->name());
    if (objectType()->canGather() &&
        distance(location(), client.character().location()) > Server::ACTION_DISTANCE) {

        text.push_back("Out of range");
    }
    return text;
}

void ClientObject::playGatherSound() const {
    Mix_Chunk *sound = objectType()->gatherSound();
    if (sound) {
        Mix_PlayChannel(Client::PLAYER_ACTION_CHANNEL, sound, -1);
    }
}
