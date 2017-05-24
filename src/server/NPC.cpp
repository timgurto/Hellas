#include <cassert>

#include "NPC.h"
#include "Server.h"

NPC::NPC(const NPCType *type, const Point &loc):
    Entity(type, loc, type->maxHealth()),
    _state(IDLE)
{}

void NPC::update(ms_t timeElapsed){
    if (health() > 0){
        processAI(timeElapsed); // May call Entity::update()
    }
}

void NPC::onHealthChange(){
    const Server &server = *Server::_instance;
    for (const User *user: server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_ENTITY_HEALTH, makeArgs(serial(), health()));
}

void NPC::onDeath(){
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

    Entity::onDeath();
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
        break; // Entity::update() will handle it
    }
      
    Entity::update(timeElapsed);
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
        server.sendMessage(client, SV_ENTITY_HEALTH, makeArgs(serial(), health()));

    // Loot
    if (!_loot.empty())
        server.sendMessage(client, SV_LOOTABLE, makeArgs(serial()));
}

void NPC::describeSelfToNewWatcher(const User &watcher) const{
    _loot.sendContentsToUser(watcher, serial());
}

void NPC::alertWatcherOnInventoryChange(const User &watcher, size_t slot) const{
    _loot.sendSingleSlotToUser(watcher, serial(), slot);

    const Server &server = Server::instance();
    if (_loot.empty())
        server.sendMessage(watcher.socket(), SV_NOT_LOOTABLE, makeArgs(serial()));
}

ServerItem::Slot *NPC::getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user){
    const Server &server = Server::instance();
    const Socket &socket = user.socket();

    if (_loot.empty()){
        server.sendMessage(socket, SV_EMPTY_SLOT);
        return nullptr;
    }

    if (!server.isEntityInRange(socket, user, this))
        return nullptr;

    if (! _loot.isValidSlot(slotNum)) {
        server.sendMessage(socket, SV_INVALID_SLOT);
        return nullptr;
    }

    ServerItem::Slot &slot = _loot.at(slotNum);
    if (slot.first == nullptr){
        server.sendMessage(socket, SV_EMPTY_SLOT);
        return nullptr;
    }

    return &slot;
}
