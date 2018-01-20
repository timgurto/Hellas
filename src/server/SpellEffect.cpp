#include "Server.h"
#include "SpellEffect.h"

void SpellEffect::setFunction(const std::string & functionName) {
    auto it = functionMap.find(functionName);
    if (it != functionMap.end())
        _function = it->second;
}


SpellEffect::FunctionMap SpellEffect::functionMap = {
    { "doDirectDamage", doDirectDamage },
    { "doDirectDamageWithModifiedThreat", doDirectDamageWithModifiedThreat },
    { "heal", heal },
    { "buff", buff },
    { "debuff", debuff },
    { "scaleThreat", scaleThreat }
};

SpellEffect::FlagMap SpellEffect::aggressionMap = {
    { doDirectDamage, true },
    { doDirectDamageWithModifiedThreat , true },
    { heal, false },
    { buff, false },
    { debuff, true },
    { scaleThreat, false }
};

bool SpellEffect::isAggressive() const {
    if (!exists())
        return false;
    auto it = aggressionMap.find(_function);
    if (it == aggressionMap.end())
        return false;
    auto isThisEffectAggressive = it->second;
    return isThisEffectAggressive;
}

CombatResult SpellEffect::doDirectDamage(const SpellEffect &effect, Entity &caster, Entity &target) {
    auto effectWithDefaultThreat = effect;
    effectWithDefaultThreat._args.d1 = 1.0; // Threat
    return doDirectDamageWithModifiedThreat(effectWithDefaultThreat, caster, target);
}

CombatResult SpellEffect::doDirectDamageWithModifiedThreat(const SpellEffect &effect, Entity &caster, Entity &target) {
    auto outcome = caster.generateHitAgainst(target, DAMAGE, effect._school, effect._range);
    if (outcome == MISS || outcome == DODGE)
        return outcome;

    auto rawDamage = static_cast<double>(effect._args.i1);
    rawDamage += caster.stats().magicDamage;

    if (outcome == CRIT)
        rawDamage *= 2;

    auto levelDiff = target.level() - caster.level();

    auto resistance = target.stats().resistanceByType(effect._school) + levelDiff;
    resistance = max(0, min(100, resistance));
    auto resistanceMultiplier = (100 - resistance) / 100.0;
    rawDamage *= resistanceMultiplier;

    auto damage = chooseRandomSpellMagnitude(rawDamage);

    if (outcome == BLOCK) {
        auto blockAmount = target.stats().blockValue;
        if (blockAmount >= damage)
            damage = 0;
        else
            damage -= blockAmount;
    }

    target.reduceHealth(damage);
    auto threatScaler = effect._args.d1;
    auto threat = toInt(damage * threatScaler);
    target.onAttackedBy(caster, threat);

    return outcome;
}

CombatResult SpellEffect::heal(const SpellEffect &effect, Entity &caster, Entity &target) {
    auto outcome = caster.generateHitAgainst(target, HEAL, effect._school, effect._range);

    auto rawAmountToHeal = static_cast<double>(effect._args.i1);
    rawAmountToHeal += caster.stats().healing;

    if (outcome == CRIT)
        rawAmountToHeal *= 2;

    auto amountToHeal = chooseRandomSpellMagnitude(rawAmountToHeal);
    target.healBy(amountToHeal);

    return outcome;
}

CombatResult SpellEffect::scaleThreat(const SpellEffect &effect, Entity &caster, Entity &target) {
    auto outcome = caster.generateHitAgainst(target, THREAT_MOD, effect._school, effect._range);
    if (outcome == MISS)
        return outcome;

    auto multiplier = effect._args.d1;

    if (outcome == CRIT)
        multiplier = multiplier * multiplier;

    target.scaleThreatAgainst(caster, multiplier);

    return outcome;
}

CombatResult SpellEffect::buff(const SpellEffect &effect, Entity & caster, Entity & target) {
    const auto &server = Server::instance();
    const auto *buffType = server.getBuffByName(effect._args.s1);
    if (buffType == nullptr)
        return FAIL;

    target.applyBuff(*buffType, caster);

    return HIT;
}

CombatResult SpellEffect::debuff(const SpellEffect &effect, Entity & caster, Entity & target) {
    const auto &server = Server::instance();
    const auto *debuffType = server.getBuffByName(effect._args.s1);
    if (debuffType == nullptr)
        return FAIL;

    auto outcome = caster.generateHitAgainst(target, DEBUFF, effect._school, effect._range);

    target.applyDebuff(*debuffType, caster);

    return HIT;
}

Hitpoints SpellEffect::chooseRandomSpellMagnitude(double raw) {
    const auto STANDARD_DEVIATION_MULTIPLIER = double{ 0.1 };
    auto sd = raw * STANDARD_DEVIATION_MULTIPLIER;
    auto bellCurve = NormalVariable{ raw, sd };
    auto randomMagnitude = bellCurve.generate();
    return toInt(randomMagnitude);
}
