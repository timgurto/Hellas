#include <cassert>

#include "NPC.h"
#include "Server.h"

NPC::NPC(const NPCType *type, const Point &loc):
    Entity(type, loc, type->maxHealth()),
    _state(IDLE)
{
    _loot.reset(new Loot);
}

void NPC::update(ms_t timeElapsed){
    if (health() > 0){
        processAI(timeElapsed); // May call Entity::update()
    }
}

CombatResult NPC::generateHitAgainst(const Entity &target, CombatType type, SpellSchool school, px_t range) const {
    const auto
        MISS_CHANCE = Percentage{ 5 },
        BASE_DODGE_CHANCE = Percentage{ 5 },
        BASE_BLOCK_CHANCE = Percentage{ 5 },
        CRIT_CHANCE = Percentage{ 5 };

    auto roll = rand() % 100;

    // Miss
    if (combatTypeCanHaveOutcome(type, MISS, school, range)) {
        if (roll < MISS_CHANCE)
            return MISS;
        roll -= MISS_CHANCE;
    }

    // Dodge
    auto dodgeChance = BASE_DODGE_CHANCE + target.bonusDodge();
    if (combatTypeCanHaveOutcome(type, DODGE, school, range)) {
        if (roll < dodgeChance)
            return DODGE;
        roll -= dodgeChance;
    }

    // Block
    auto blockChance = BASE_BLOCK_CHANCE + target.bonusBlock();
    if (target.canBlock() && combatTypeCanHaveOutcome(type, BLOCK, school, range)) {
        if (roll < blockChance)
            return BLOCK;
        roll -= blockChance;
    }

    // Crit
    if (combatTypeCanHaveOutcome(type, CRIT, school, range)) {
        if (roll < CRIT_CHANCE)
            return CRIT;
        roll -= CRIT_CHANCE;
    }

    return HIT;
}

void NPC::onHealthChange(){
    const Server &server = *Server::_instance;
    for (const User *user: server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_ENTITY_HEALTH, makeArgs(serial(), health()));
}

void NPC::onDeath(){
    Server &server = *Server::_instance;
    server.forceAllToUntarget(*this);

    npcType()->lootTable().instantiate(*_loot);
    if (! _loot->empty())
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

void NPC::onAttackedBy(Entity &attacker) {
    _recentAttacker = &attacker;
}

void NPC::processAI(ms_t timeElapsed){
    const auto
        VIEW_RANGE = 50_px,
        ATTACK_RANGE = 5_px,
        // Assumption: this is farther than any ranged attack/spell can reach.
        CONTINUE_ATTACKING_RANGE = Podes{ 35 }.toPixels(); 

    double distToTarget = (target() == nullptr) ? 0 :
            distance(collisionRect(), target()->collisionRect());

    // Transition if necessary
    switch(_state){
    case IDLE:
        if (attack() == 0) // NPCs that can't attack won't try.
            break;

        // React to recent attacker
        if (_recentAttacker != nullptr) {
            target(_recentAttacker);
            _state = CHASE;
            _recentAttacker = nullptr;
            break;
        }

        // Look for nearby users to attack
        for (User *user : Server::_instance->findUsersInArea(location(), VIEW_RANGE)){
            if (distance(collisionRect(), user->collisionRect()) <= VIEW_RANGE){
                target(dynamic_cast<Entity *>(user));
                _state = CHASE;
                break;
            }
        }
        break;

    case CHASE:
        // React to recent attacker
        if (_recentAttacker != nullptr) {
            target(_recentAttacker);
            _state = CHASE;
            _recentAttacker = nullptr;
            break;
        }

        // Target has disappeared
        if (target() == nullptr) {
            _state = IDLE;
            break;
        }
        
        // Target has run out of range: give up
        if (distToTarget > CONTINUE_ATTACKING_RANGE) {
            _state = IDLE;
            target(nullptr);
            break;
        }
        
        // Target is enough for me to attack
        if (distToTarget <= ATTACK_RANGE) {
            _state = ATTACK;
        }
        break;

    case ATTACK:
        // React to recent attacker
        if (_recentAttacker != nullptr) {
            target(_recentAttacker);
            _state = CHASE;
            _recentAttacker = nullptr;
            break;
        }

        // Target has disappeared
        if (target() == nullptr) {
            _state = IDLE;
            break;
        }

        // Target has run out of attack range: chase
        if (distToTarget > ATTACK_RANGE) {
            _state = CHASE;
            break;
        }

        // Target is dead
        if (target()->health() == 0){
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

void NPC::sendInfoToClient(const User &targetUser) const {
    const Server &server = Server::instance();
    const Socket &client = targetUser.socket();

    server.sendMessage(client, SV_OBJECT, makeArgs(serial(), location().x, location().y,
                                                   type()->id()));

    // Hitpoints
    if (health() < maxHealth())
        server.sendMessage(client, SV_ENTITY_HEALTH, makeArgs(serial(), health()));

    // Loot
    if (!_loot->empty())
        server.sendMessage(client, SV_LOOTABLE, makeArgs(serial()));
}

void NPC::describeSelfToNewWatcher(const User &watcher) const{
    _loot->sendContentsToUser(watcher, serial());
}

void NPC::alertWatcherOnInventoryChange(const User &watcher, size_t slot) const{
    _loot->sendSingleSlotToUser(watcher, serial(), slot);

    const Server &server = Server::instance();
    if (_loot->empty())
        server.sendMessage(watcher.socket(), SV_NOT_LOOTABLE, makeArgs(serial()));
}

ServerItem::Slot *NPC::getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user){
    const Server &server = Server::instance();
    const Socket &socket = user.socket();

    if (_loot->empty()){
        server.sendMessage(socket, SV_EMPTY_SLOT);
        return nullptr;
    }

    if (!server.isEntityInRange(socket, user, this))
        return nullptr;

    if (! _loot->isValidSlot(slotNum)) {
        server.sendMessage(socket, SV_INVALID_SLOT);
        return nullptr;
    }

    ServerItem::Slot &slot = _loot->at(slotNum);
    if (slot.first == nullptr){
        server.sendMessage(socket, SV_EMPTY_SLOT);
        return nullptr;
    }

    return &slot;
}
