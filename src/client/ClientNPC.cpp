#include <cassert>

#include "Client.h"
#include "ClientNPC.h"

extern Renderer renderer;

ClientNPC::ClientNPC(size_t serial, const ClientNPCType *type, const MapPoint &loc):
ClientObject(serial, type, loc)
{}

bool ClientNPC::canBeAttackedByPlayer() const{
    if (! ClientCombatant::canBeAttackedByPlayer())
        return false;
    if (npcType()->isCivilian())
        return false;
    return true;
}

void ClientNPC::draw(const Client & client) const {
    ClientObject::draw(client);

    // Draw gear
    if (npcType()->hasGear())
    for (const auto &pair : ClientItem::drawOrder()) {
        const ClientItem *item = npcType()->gear(pair.second);
        if (item)
            item->draw(location());
    }
}
