#include <cassert>

#include "Client.h"
#include "ClientNPC.h"

extern Renderer renderer;

ClientNPC::ClientNPC(size_t serial, const ClientNPCType *type, const Point &loc):
ClientObject(serial, type, loc)
{}

void ClientNPC::draw(const Client &client) const{
    ClientObject::draw(client);

    drawHealthBarIfAppropriate(location(), height());

    if (isDebug()) {
        renderer.setDrawColor(Color::WHITE);
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
