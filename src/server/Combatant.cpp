#include <cassert>

#include "Combatant.h"
#include "Server.h"

Combatant::Combatant(const ObjectType *type, const Point &loc, health_t health):
Object(type, loc),
_health(health),
_attackTimer(0),
_target(nullptr)
{}

void Combatant::reduceHealth(int damage){
    if (damage >= static_cast<int>(_health)) {
        _health = 0;
        onDeath();
    } else if (damage != 0) {
        _health -= damage;
    }
}

void Combatant::update(ms_t timeElapsed){
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
        if (classTag() == 'u' && target()->classTag() == 'n'){
            msgCode = SV_PLAYER_HIT_NPC;
            args = makeArgs(
                    dynamic_cast<const User *>(this)->name(),
                    dynamic_cast<const NPC *>(target())->serial());
        } else if (classTag() == 'n' && target()->classTag() == 'u'){
            msgCode = SV_NPC_HIT_PLAYER;
            args = makeArgs(
                    dynamic_cast<const NPC *>(this)->serial(),
                    dynamic_cast<const User *>(target())->name());
        } else {
            assert(false);
            return;
        }
        for (const User *user : server.findUsersInArea(locus)){
            server.sendMessage(user->socket(), msgCode, args);
        }

        // Reset timer
        _attackTimer = attackTime();
    }
}
