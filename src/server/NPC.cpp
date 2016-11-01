#include "NPC.h"
#include "Server.h"

const ms_t NPC::CORPSE_TIME = 600000; // 10 minutes
const size_t NPC::LOOT_CAPACITY = 8;

NPC::NPC(const NPCType *type, const Point &loc):
Combatant(type, loc, type->maxHealth()),
_state(IDLE){}

void NPC::update(ms_t timeElapsed){
    Object::update(timeElapsed);

    if (health() == 0){
        if (timeElapsed < _corpseTime)
            _corpseTime -= timeElapsed;
        else
            markForRemoval();
    }

    processAI(timeElapsed);
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

void NPC::processAI(ms_t timeElapsed){
    static const px_t
        VIEW_RANGE = 50,
        ATTACK_RANGE = 5;

    // Transition if necessary
    switch(_state){
    case IDLE:
        for (User *user : Server::_instance->findUsersInArea(location(), VIEW_RANGE)){
            if (distance(location(), user->location()) <= VIEW_RANGE){
                target(dynamic_cast<Combatant *>(user));
                _state = CHASE;
                break;
            }
        }
        break;

    case CHASE:
        if (distance(location(), target()->location()) > VIEW_RANGE){
            _state = IDLE;
            target(nullptr);
        } else  if (distance(location(), target()->location()) <= ATTACK_RANGE){
            _state = ATTACK;
        }
        break;

    case ATTACK:
        if (distance(location(), target()->location()) > ATTACK_RANGE){
            _state = CHASE;
        } else  if (target()->health() == 0){
            _state = IDLE;
            target(nullptr);
        }
        break;
    }

    // Act
    switch(_state){
    case CHASE:
        // Move towards player
        break;

    case IDLE:
    case ATTACK:
        break;
    }
}
