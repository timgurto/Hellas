// (C) 2016 Tim Gurto

#include "NPC.h"
#include "Server.h"

const ms_t NPC::CORPSE_TIME = 600000; // 10 minutes

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

void NPC::kill(){
    _corpseTime = CORPSE_TIME;
    Server::_instance->forceUntarget(*this);
}
