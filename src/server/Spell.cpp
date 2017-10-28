#include <string>

#include "Spell.h"

void Spell::setFunction(const std::string & functionName) {
    auto it = functionMap.find(functionName);
    if (it != functionMap.end())
        _function = it->second;
}

Spell::Outcome Spell::performAction(Entity &caster, Entity &target) const {
    if (_function == nullptr)
        return FAIL;
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
