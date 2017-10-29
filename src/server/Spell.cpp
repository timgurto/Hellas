#include <string>

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

    // Target check
    if (caster.classTag() == 'u') {
        const auto &asUser = dynamic_cast<User &>(caster);
        if (!target.canBeAttackedBy(asUser)) {
            // TODO: Send message
            return FAIL;
        }
    }

    // Energy check
    auto cost = Energy{ 30 };
    if (caster.energy() < cost) {
        // TODO: Send message
        return FAIL;
    }
    auto newEnergy = caster.energy() - cost;
    caster.energy(newEnergy);
    caster.onEnergyChange();

    return _function(caster, target, _args);
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

    if (outcome != MISS)
        target.reduceHealth(damage);

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