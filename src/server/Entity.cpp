#include <cassert>

#include "Entity.h"
#include "Server.h"
#include "Spawner.h"

Entity::Entity(const EntityType *type, const Point &loc, health_t health):
    _type(type),
    _serial(generateSerial()),
    _spawner(nullptr),

    _location(loc),
    _lastLocUpdate(SDL_GetTicks()),

    _health(health),
    _attackTimer(0),
    _target(nullptr)
{}

Entity::Entity(size_t serial): // For set/map lookup ONLY
    _type(nullptr),
    _serial(serial)
{}

Entity::Entity(const Point &loc): // For set/map lookup ONLY
    _type(nullptr),
    _location(loc),
    _serial(0)
{}

Entity::~Entity(){
    if (_spawner != nullptr)
        _spawner->scheduleSpawn();
}

bool Entity::compareSerial::operator()( const Entity *a, const Entity *b){
    return a->_serial < b->_serial;
}

bool Entity::compareXThenSerial::operator()( const Entity *a, const Entity *b){
    if (a->_location.x != b->_location.x)
        return a->_location.x < b->_location.x;
    return a->_serial < b->_serial;
}

bool Entity::compareYThenSerial::operator()( const Entity *a, const Entity *b){
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

void Entity::reduceHealth(int damage){
    if (damage >= static_cast<int>(_health)) {
        _health = 0;
        onDeath();
    } else if (damage != 0) {
        _health -= damage;
    }
}

void Entity::update(ms_t timeElapsed){
    if (_attackTimer > timeElapsed)
        _attackTimer -= timeElapsed;
    else
        _attackTimer = 0;

    assert(target()->health() > 0);

    if (_attackTimer > 0)
        return;

    // Check if within range
    if (distance(collisionRect(), target()->collisionRect()) <= Server::ACTION_DISTANCE){

        // Reduce target health (to minimum 0)
        target()->reduceHealth(attack());
        target()->onHealthChange();

        // Alert nearby clients
        const Server &server = Server::instance();
        Point locus = midpoint(location(), target()->location());
        MessageCode msgCode;
        std::string args;
        char
            attackerTag = classTag(),
            defenderTag = target()->classTag();
        if (attackerTag == 'u' && defenderTag == 'n'){
            msgCode = SV_PLAYER_HIT_NPC;
            args = makeArgs(
                    dynamic_cast<const User *>(this)->name(),
                    dynamic_cast<const NPC *>(target())->serial());
        } else if (attackerTag == 'n' && defenderTag == 'u'){
            msgCode = SV_NPC_HIT_PLAYER;
            args = makeArgs(
                    dynamic_cast<const NPC *>(this)->serial(),
                    dynamic_cast<const User *>(target())->name());
        } else if (attackerTag == 'u' && defenderTag == 'u') {
            msgCode = SV_PLAYER_HIT_PLAYER;
            args = makeArgs(
                    dynamic_cast<const User *>(this)->name(),
                    dynamic_cast<const User *>(target())->name());
        } else {
            assert(false);
        }
        for (const User *user : server.findUsersInArea(locus)){
            server.sendMessage(user->socket(), msgCode, args);
        }

        // Reset timer
        _attackTimer = attackTime();
    }
}
