#include <cassert>

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
    } else {
        processAI(timeElapsed);
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
    assert(hasContainer());
    npcType()->lootTable().instantiate(container().raw());
    for (const User *user : server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_LOOTABLE, makeArgs(serial()));

    /*
    Schedule a respawn, if this NPC came from a spawner.
    For non-NPCs, this happens in onRemove().  The object's _spawner is cleared afterwards to avoid
    onRemove() from doing so here.
    */
    if (spawner() != nullptr){
        spawner()->scheduleSpawn();
        spawner(nullptr);
    }
}

void NPC::processAI(ms_t timeElapsed){
    static const px_t
        VIEW_RANGE = 50,
        ATTACK_RANGE = 5;
    double distToTarget = (target() == nullptr) ? 0 :
            distance(collisionRect(), target()->collisionRect());

    // Transition if necessary
    switch(_state){
    case IDLE:
        if (attack() == 0) // NPCs that can't attack won't try.
            break;
        for (User *user : Server::_instance->findUsersInArea(location(), VIEW_RANGE)){
            if (distance(collisionRect(), user->collisionRect()) <= VIEW_RANGE){
                target(dynamic_cast<Combatant *>(user));
                _state = CHASE;
                break;
            }
        }
        break;

    case CHASE:
        if (target() == nullptr) {
            _state = IDLE;
        } else if (distToTarget > VIEW_RANGE){
            _state = IDLE;
            target(nullptr);
        } else  if (distToTarget <= ATTACK_RANGE){
            _state = ATTACK;
        }
        break;

    case ATTACK:
        if (target() == nullptr){
            _state = IDLE;
        } else if (distToTarget > ATTACK_RANGE){
            _state = CHASE;
        } else  if (target()->health() == 0){
            _state = IDLE;
            target(nullptr);
        }
        break;
    }

    // Act
    switch(_state){
    case IDLE:
        break;

    case CHASE:
        // Move towards player
        updateLocation(target()->location());
        break;

    case ATTACK:
        Combatant::update(timeElapsed);
        break;
    }
}

bool NPC::collides() const{
    return Object::collides() && health() > 0;
}
