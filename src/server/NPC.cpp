#include "NPC.h"
#include "Server.h"

const ms_t NPC::CORPSE_TIME = 600000; // 10 minutes
const size_t NPC::LOOT_CAPACITY = 8;

NPC::NPC(const NPCType *type, const Point &loc):
Object(type, loc),
Combatant(type->maxHealth()){}

void NPC::update(ms_t timeElapsed){
    Object::update(timeElapsed);

    if (health() == 0){
        if (timeElapsed < _corpseTime)
            _corpseTime -= timeElapsed;
        else
            markForRemoval();
    }
}

void NPC::onHealthChange(){
    const Server &server = *Server::_instance;
    for (const User *user: server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_NPC_HEALTH, makeArgs(serial(), health()));
}

void NPC::onDeath(){
    _corpseTime = CORPSE_TIME;
    Server &server = *Server::_instance;
    server.forceUntarget(*this);
    npcType()->lootTable().instantiate(_container);
    for (const User *user : server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_LOOTABLE, makeArgs(serial()));
}
