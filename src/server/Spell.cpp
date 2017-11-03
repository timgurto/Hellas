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

    return _function(*this, caster, target, _args);
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

Hitpoints Spell::chooseRandomSpellMagnitude(double raw) {
    const auto STANDARD_DEVIATION_MULTIPLIER = double{ 0.1 };
    auto sd = raw * STANDARD_DEVIATION_MULTIPLIER;
    auto bellCurve = NormalVariable{ raw, sd };
    auto randomMagnitude = bellCurve.generate();
    return toInt(randomMagnitude);
}

Spell::FlagMap Spell::aggressionMap = {
    { doDirectDamage, true },
    { heal, false }
};

Spell::FunctionMap Spell::functionMap = {
    { "doDirectDamage", doDirectDamage },
    { "heal", heal }
};

CombatResult Spell::doDirectDamage(const Spell &spell, Entity &caster, Entity &target, const Args &args) {
    auto outcome = caster.generateHit(DAMAGE, spell._range);
    if (outcome == MISS)
        return MISS;

    auto rawDamage = static_cast<double>(args[0]);
    rawDamage += caster.bonusMagicDamage();

    if (outcome == CRIT)
        rawDamage *= 2;

    auto resistance = target.getResistance(spell._school);
    auto resistanceMultiplier = (100 - resistance) / 100.0;
    rawDamage += resistanceMultiplier;

    auto damage = chooseRandomSpellMagnitude(rawDamage);

    target.reduceHealth(damage);
    target.onAttackedBy(caster);

    return outcome;
}

CombatResult Spell::heal(const Spell &spell, Entity &caster, Entity &target, const Args &args) {
    auto outcome = caster.generateHit(HEAL, spell._range);

    auto rawAmountToHeal = static_cast<double>(args[0]);

    if (outcome == CRIT)
        rawAmountToHeal *= 2;

    auto amountToHeal = chooseRandomSpellMagnitude(rawAmountToHeal);
    target.healBy(amountToHeal);

    return outcome;
}
