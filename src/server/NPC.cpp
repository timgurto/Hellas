#include <cassert>

#include "NPC.h"
#include "Server.h"

const ms_t NPC::CORPSE_TIME = 600000; // 10 minutes
const size_t NPC::LOOT_CAPACITY = 8;

NPC::NPC(const NPCType *type, const Point &loc):
    Entity(type, loc, type->maxHealth()),
    _state(IDLE)
{}

void NPC::update(ms_t timeElapsed){
    if (health() == 0){
        if (timeElapsed < _corpseTime)
            _corpseTime -= timeElapsed;
        else
            markForRemoval();
    } else {
        processAI(timeElapsed); // May call Entity::update()
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
    server.forceAllToUntarget(*this);

    npcType()->lootTable().instantiate(_loot);
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
                target(dynamic_cast<Entity *>(user));
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
        Entity::update(timeElapsed);
        break;
    }
}

bool NPC::collides() const{
    return Entity::collides() && health() > 0;
}

void NPC::sendInfoToClient(const User &targetUser) const {
    const Server &server = Server::instance();
    const Socket &client = targetUser.socket();

    server.sendMessage(client, SV_OBJECT, makeArgs(serial(), location().x, location().y,
                                                   type()->id()));

    // Health
    if (health() < maxHealth())
        server.sendMessage(client, SV_NPC_HEALTH, makeArgs(serial(), health()));

    // Loot
    if (!_loot.empty())
        server.sendMessage(client, SV_LOOTABLE, makeArgs(serial()));
}

void NPC::describeSelfToNewWatcher(const User &watcher) const{
    _loot.sendContentsToUser(watcher, serial());
}
