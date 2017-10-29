#include <string>

#include "Server.h"
#include "Spell.h"
#include "User.h"

void Spell::setFunction(const std::string & functionName) {
    auto it = functionMap.find(functionName);
    if (it != functionMap.end())
        _function = it->second;
}

Spell::Outcome Spell::performAction(Entity &caster, Entity &target) const {
    if (_function == nullptr)
        return FAIL;

    const Server &server = Server::instance();
    const auto *casterAsUser = dynamic_cast<const User *>(&caster);

    // Target check
    if (!isTargetValid(caster, target)) {
        if (casterAsUser)
            server.sendMessage(casterAsUser->socket(), SV_INVALID_SPELL_TARGET);
        return FAIL;
    }

    if (target.isDead()){
        if (casterAsUser)
            server.sendMessage(casterAsUser->socket(), SV_TARGET_DEAD);
        return FAIL;
    }

    // Range check
    const auto SPELL_RANGE = 200_px; // Must be reconciled with NPCs' CONTINUE_ATTACKING_RANGE
    if (distance(caster.location(), target.location()) > SPELL_RANGE) {
        if (casterAsUser)
            server.sendMessage(casterAsUser->socket(), SV_TOO_FAR);
        return FAIL;
    }

    // Energy check
    if (caster.energy() < _cost) {
        if (casterAsUser)
            server.sendMessage(casterAsUser->socket(), SV_NOT_ENOUGH_ENERGY);
        return FAIL;
    }

    auto newEnergy = caster.energy() - _cost;
    caster.energy(newEnergy);
    caster.onEnergyChange();

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

Spell::Outcome Spell::doDirectDamage(Entity &caster, Entity &target, const Args &args) {
    auto outcome = HIT;
    auto roll = rand() % 100;
    if (roll < 5)
        outcome = CRIT;
    else if (roll < 10)
        outcome = MISS;

    auto damage = args[0];
    if (outcome == CRIT)
        damage *= 2;

    if (outcome != MISS) {
        target.reduceHealth(damage);
        target.onHealthChange();
        target.onAttackedBy(caster);
    }

    return outcome;
}

Spell::Outcome Spell::heal(Entity &caster, Entity &target, const Args &args) {
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