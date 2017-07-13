#include <cassert>

#include "Client.h"
#include "ClientNPC.h"

extern Renderer renderer;

ClientNPC::ClientNPC(size_t serial, const ClientNPCType *type, const Point &loc):
ClientObject(serial, type, loc)
{}

const Texture &ClientNPC::cursor(const Client &client) const{
    if (isAlive())
        return client.cursorAttack();
    if (lootable())
        return  client.cursorContainer();
    return client.cursorNormal();
}

bool ClientNPC::isFlat() const{
    return Sprite::isFlat() || health() == 0;
}

bool ClientNPC::canBeAttackedByPlayer() const{
    if (! ClientCombatant::canBeAttackedByPlayer())
        return false;
    return true;
}
