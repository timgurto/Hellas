#include <cassert>

#include "Entity.h"
#include "Server.h"
#include "Spawner.h"
#include "../util.h"

const px_t Entity::DEFAULT_ATTACK_RANGE = Podes{ 4 }.toPixels();

Entity::Entity(const EntityType *type, const Point &loc, Hitpoints health):
    _type(type),
    _serial(generateSerial()),
    _spawner(nullptr),

    _location(loc),
    _lastLocUpdate(SDL_GetTicks()),

    _health(health),
    _attackTimer(0),
    _target(nullptr),
    _loot(nullptr)
{}

Entity::Entity(size_t serial): // For set/map lookup ONLY
    _type(nullptr),
    _serial(serial),
    _spawner(nullptr),
    _loot(nullptr)
{}

Entity::Entity(const Point &loc): // For set/map lookup ONLY
    _type(nullptr),
    _location(loc),
    _serial(0),
    _spawner(nullptr),
    _loot(nullptr)
{}

Entity::~Entity(){
    if (_spawner != nullptr)
        _spawner->scheduleSpawn();
}

bool Entity::compareSerial::operator()( const Entity *a, const Entity *b) const{
    return a->_serial < b->_serial;
}

bool Entity::compareXThenSerial::operator()( const Entity *a, const Entity *b) const{
    if (a->_location.x != b->_location.x)
        return a->_location.x < b->_location.x;
    return a->_serial < b->_serial;
}

bool Entity::compareYThenSerial::operator()( const Entity *a, const Entity *b) const{
    if (a->_location.y != b->_location.y)
        return a->_location.y < b->_location.y;
    return a->_serial < b->_serial;
}

size_t Entity::generateSerial() {
    static size_t currentSerial = Server::STARTING_SERIAL;
    return currentSerial++;
}

void Entity::markForRemoval(){
    Server::_instance->_entitiesToRemove.push_back(this);
}

bool Entity::combatTypeCanHaveOutcome(CombatType type, CombatResult outcome, SpellSchool school,
        px_t range) {
    if (type == HEAL && outcome == MISS)
        return false;
    if (outcome == DODGE && range > Podes::MELEE_RANGE)
        return false;
    if (outcome == BLOCK && school.isMagic())
        return false;

    return true;
}

void Entity::sendGotHitMessageTo(const User & user) const {
    Server::_instance->sendMessage(user.socket(), SV_ENTITY_WAS_HIT, makeArgs(serial()));
}

void Entity::reduceHealth(int damage) {
    if (damage == 0)
        return;
    if (damage >= static_cast<int>(_health)) {
        _health = 0;
        onHealthChange();
        onDeath();
    } else {
        _health -= damage;
        onHealthChange();
    }

    assert(_health <= this->maxHealth());
}

void Entity::reduceEnergy(int amount) {
    if (amount == 0)
        return;
    if (amount > static_cast<int>(_energy))
        amount = _energy;
    _energy -= amount;
    onEnergyChange();
}

void Entity::healBy(Hitpoints amount) {
    auto newHealth = min(health() + amount, maxHealth());
    _health = newHealth;
    onHealthChange();
}

void Entity::update(ms_t timeElapsed){
    if (_attackTimer > timeElapsed)
        _attackTimer -= timeElapsed;
    else
        _attackTimer = 0;

    if (health() == 0){
        if (_corpseTime > timeElapsed)
            _corpseTime -= timeElapsed;
        else
            markForRemoval();
        return;
    }

    Entity *pTarget = target();
    if (pTarget == nullptr)
        return;

    assert(pTarget->health() > 0);

    if (_attackTimer > 0)
        return;

    // Reset timer
    _attackTimer = attackTime();

    // Check if within range
    if (distance(collisionRect(), pTarget->collisionRect()) <= attackRange()){
        const Server &server = Server::instance();
        Point locus = midpoint(location(), pTarget->location());

        auto outcome = generateHitAgainst(*pTarget, DAMAGE, SpellSchool::PHYSICAL, attackRange());

        switch (outcome) {
        // These cases return
        case MISS:
            for (const User *userToInform : server.findUsersInArea( locus ))
                server.sendMessage( userToInform->socket(), SV_SHOW_MISS_AT, makeArgs(
                    pTarget->location().x, pTarget->location().y ) );
            return;
        case DODGE:
            for (const User *userToInform : server.findUsersInArea(locus))
                server.sendMessage(userToInform->socket(), SV_SHOW_DODGE_AT, makeArgs(
                    pTarget->location().x, pTarget->location().y));
            return;

        // These cases continue on
        case CRIT:
            for (const User *userToInform : server.findUsersInArea(locus))
                server.sendMessage(userToInform->socket(), SV_SHOW_CRIT_AT, makeArgs(
                    pTarget->location().x, pTarget->location().y));
            break;
        case BLOCK:
            for (const User *userToInform : server.findUsersInArea(locus))
                server.sendMessage(userToInform->socket(), SV_SHOW_BLOCK_AT, makeArgs(
                    pTarget->location().x, pTarget->location().y));
            break;
        }

        auto damage = attack();
        if (outcome == CRIT)
            damage *= 2;

        else if (outcome == BLOCK) {
            const auto BLOCK_AMOUNT = Hitpoints{ 5 };
            if (BLOCK_AMOUNT >= damage)
                damage = 0;
            else
                damage -= BLOCK_AMOUNT;
        }

        pTarget->reduceHealth(damage);

        // Give target opportunity to react
        pTarget->onAttackedBy(*this);

        // Alert nearby clients
        MessageCode msgCode;
        std::string args;
        char
            attackerTag = classTag(),
            defenderTag = pTarget->classTag();
        if (attackerTag == 'u' && defenderTag != 'u'){
            msgCode = SV_PLAYER_HIT_ENTITY;
            args = makeArgs(
                    dynamic_cast<const User *>(this)->name(),
                    pTarget->serial());
        } else if (attackerTag != 'u' && defenderTag == 'u'){
            msgCode = SV_ENTITY_HIT_PLAYER;
            args = makeArgs(
                    serial(),
                    dynamic_cast<const User *>(pTarget)->name());
        } else if (attackerTag == 'u' && defenderTag == 'u') {
            msgCode = SV_PLAYER_HIT_PLAYER;
            args = makeArgs(
                    dynamic_cast<const User *>(this)->name(),
                    dynamic_cast<const User *>(pTarget)->name());
        } else {
            assert(false);
        }
        for (const User *userToInform : server.findUsersInArea(locus)){
            server.sendMessage(userToInform->socket(), msgCode, args);

            // Show notable outcomes
            if (outcome == CRIT)
                server.sendMessage(userToInform->socket(), SV_SHOW_CRIT_AT, makeArgs(
                    pTarget->location().x, pTarget->location().y));
        }
    }
}

void Entity::onDeath(){
    if (timeToRemainAsCorpse() == 0)
        markForRemoval();
    else
        startCorpseTimer();
}

void Entity::startCorpseTimer(){
    _corpseTime = timeToRemainAsCorpse();
}

void Entity::addWatcher(const std::string &username){
    _watchers.insert(username);
    Server::debug() << username << " is now watching an object." << Log::endl;
}

void Entity::removeWatcher(const std::string &username){
    _watchers.erase(username);
    Server::debug() << username << " is no longer watching an object." << Log::endl;
}
