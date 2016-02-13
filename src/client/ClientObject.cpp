// (C) 2015 Tim Gurto

#include <cassert>

#include "ClientObject.h"
#include "Client.h"
#include "Renderer.h"
#include "../Color.h"
#include "../util.h"

extern Renderer renderer;

ClientObject::ClientObject(const ClientObject &rhs):
Entity(rhs),
_serial(rhs._serial){}

ClientObject::ClientObject(size_t serialArg, const EntityType *type, const Point &loc):
Entity(type, loc),
_serial(serialArg){}

void ClientObject::onRightClick(Client &client) const{
    if (objectType()->canGather()) {
        std::ostringstream oss;
        oss << '[' << CL_GATHER << ',' << _serial << ']';
        client.socket().sendMessage(oss.str());
        client.prepareAction(std::string("Gathering ") + objectType()->name());
        playGatherSound();
    }
}

std::vector<std::string> ClientObject::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back(objectType()->name());
    if (!_owner.empty())
        text.push_back(std::string("Owned by ") + _owner);
    return text;
}

void ClientObject::playGatherSound() const {
    Mix_Chunk *sound = objectType()->gatherSound();
    if (sound) {
        Mix_PlayChannel(Client::PLAYER_ACTION_CHANNEL, sound, -1);
    }
}

void ClientObject::draw(const Client &client) const{
    assert(type());
    // Highilght moused-over entity
    if (this == client.currentMouseOverEntity()) {
        if (distance(collisionRect(), client.playerCollisionRect()) <= Client::ACTION_DISTANCE)
            renderer.setDrawColor(Color::BLUE/2 + Color::WHITE/2);
        else
            renderer.setDrawColor(Color::GREY_2);
        renderer.drawRect(collisionRect() + Rect(-1, -1, 2, 2) + client.offset());
    }

    type()->drawAt(location() + client.offset());

    if (isDebug()) {
        renderer.setDrawColor(Color::GREY_2);
        renderer.drawRect(collisionRect() + client.offset());
        renderer.setDrawColor(Color::YELLOW);
        renderer.fillRect(Rect(location().x + client.offset().x,
                                location().y + client.offset().y - 1,
                                1, 3));
        renderer.fillRect(Rect(location().x + client.offset().x - 1,
                               location().y + client.offset().y,
                               3, 1));
    }
}
