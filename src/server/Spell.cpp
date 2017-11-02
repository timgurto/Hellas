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

Spell::FlagMap Spell::aggressionMap = {
    { doDirectDamage, true },
    { heal, false }
};

Spell::FunctionMap Spell::functionMap = {
    { "doDirectDamage", doDirectDamage },
    { "heal", heal }
};

CombatResult Spell::doDirectDamage(Entity &caster, Entity &target, const Args &args) {
    auto outcome = caster.generateHit(DAMAGE);
    if (outcome == MISS)
        return MISS;

    auto rawDamage = static_cast<double>(args[0]);
    rawDamage += caster.bonusMagicDamage();

    if (outcome == CRIT)
        rawDamage *= 2;

    auto sd = rawDamage * 0.15;
    auto damageBellCurve = NormalVariable{rawDamage};
    auto randomDamage = damageBellCurve.generate();
    auto damage = toInt(randomDamage);
    Server::debug()("Spell doing "s + toString(damage) + " damage"s);

    target.reduceHealth(damage);
    target.onAttackedBy(caster);

    return outcome;
}

CombatResult Spell::heal(Entity &caster, Entity &target, const Args &args) {
    auto outcome = caster.generateHit(HEAL);

    switch (outcome) {
    case CRIT:
        target.healBy(args[0] * 2);
        break;
    case HIT:
        target.healBy(args[0]);
        break;
    }

    return outcome;
}
