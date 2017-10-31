#include <string>

#include "Server.h"
#include "Spell.h"
#include "User.h"

void Spell::setFunction(const std::string & functionName) {
    auto it = functionMap.find(functionName);
    if (it != functionMap.end())
        _function = it->second;
}

CombatResult Spell::performAction(Entity &caster, Entity &target) const {
    if (_function == nullptr)
        return FAIL;

    const Server &server = Server::instance();
    const auto *casterAsUser = dynamic_cast<const User *>(&caster);

    auto skipWarnings = _targetsInArea; // Since there may be many targets, not all valid.

    // Target check
    if (!isTargetValid(caster, target)) {
        if (casterAsUser && !skipWarnings)
            server.sendMessage(casterAsUser->socket(), SV_INVALID_SPELL_TARGET);
        return FAIL;
    }

    if (target.isDead()){
        if (casterAsUser && !skipWarnings)
            server.sendMessage(casterAsUser->socket(), SV_TARGET_DEAD);
        return FAIL;
    }

    // Range check
    if (distance(caster.location(), target.location()) > _range) {
        if (casterAsUser && !skipWarnings)
            server.sendMessage(casterAsUser->socket(), SV_TOO_FAR);
        return FAIL;
    }

    return _function(caster, target, _args);
}

bool Spell::isTargetValid(const Entity &caster, const Entity &target) const {

    if (caster.classTag() != 'u')
        return false; // For now, forbid non-user casters

    if (&caster == &target)
        return _validTargets[SELF];

    const auto &casterAsUser = dynamic_cast<const User &>(caster);

    if (target.canBeAttackedBy(casterAsUser))
        return _validTargets[ENEMY];

    return _validTargets[FRIENDLY];
}

Spell::FunctionMap Spell::functionMap = {
    { "doDirectDamage", doDirectDamage },
    { "heal", heal }
};

CombatResult Spell::doDirectDamage(Entity &caster, Entity &target, const Args &args) {
    auto outcome = caster.generateHit();
    
    switch (outcome) {
    case CRIT:
        target.reduceHealth(args[0] * 2);
        target.onAttackedBy(caster);
        break;
    case HIT:
        target.reduceHealth(args[0]);
        target.onAttackedBy(caster);
        break;
    }

    return outcome;
}

CombatResult Spell::heal(Entity &caster, Entity &target, const Args &args) {
    auto outcome = HIT;
    auto roll = rand() % 100;
    if (roll < 5)
        outcome = CRIT;

    auto amountToHeal = args[0];
    if (outcome == CRIT)
        amountToHeal *= 2;

    target.healBy(amountToHeal);

    return outcome;
}