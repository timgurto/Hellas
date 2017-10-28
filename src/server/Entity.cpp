#include <cassert>

#include "Entity.h"
#include "Server.h"
#include "Spawner.h"
#include "../util.h"

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

void Entity::broadcastHealth() const {
    Server::_instance->broadcastToArea(_location, SV_ENTITY_HEALTH, makeArgs(_serial, _health));
}

void Entity::reduceHealth(int damage) {
    if (damage >= static_cast<int>(_health)) {
        _health = 0;
        broadcastHealth();
        onDeath();
    } else if (damage != 0) {
        _health -= damage;
        broadcastHealth();
    }

    assert(_health <= this->maxHealth());
}

void Entity::healBy(Hitpoints amount) {
    auto newHealth = min(health() + amount, maxHealth());
    _health = newHealth;
    broadcastHealth();
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

    // Check if within range
    if (distance(collisionRect(), pTarget->collisionRect()) <= Server::ACTION_DISTANCE){

        // Reduce target health (to minimum 0)
        pTarget->reduceHealth(attack());
        pTarget->onHealthChange();

        // Alert nearby clients
        const Server &server = Server::instance();
        Point locus = midpoint(location(), pTarget->location());
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
        }

        // Reset timer
        _attackTimer = attackTime();
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
