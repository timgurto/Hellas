#pragma once

#include <map>
#include <vector>

#include "Entity.h"
#include "SpellEffect.h"
#include "../Podes.h"
#include "../SpellSchool.h"
#include "../types.h"

class Spell {
public:
    enum TargetType {
        SELF,
        FRIENDLY,
        ENEMY,

        NUM_TARGET_TYPES
    };

    using ID = std::string;
    using Name = std::string;

    CombatResult performAction(Entity &caster, Entity &target) const;
        bool isTargetValid(const Entity &caster, const Entity &target) const;

    void name(const Name &name) { _name = name; }
    const Name &name() const { return _name; }
    void canTarget(TargetType type) { _validTargets[type] = true; }
    void cost(Energy e) { _cost = e; }
    Energy cost() const { return _cost; }
    bool shouldPlayDefenseSound() const { return _effect.isAggressive(); }
    void range(Podes r) { _range = r.toPixels(); }
    SpellEffect &effect() { return _effect; }
    const SpellEffect &effect() const { return _effect; }
    bool canCastOnlyOnSelf() const;

private:
    Name _name;
    Energy _cost = 0;
    px_t _range = Podes::MELEE_RANGE.toPixels();

    SpellEffect _effect;

    using ValidTargets = std::vector<bool>;
    ValidTargets _validTargets = ValidTargets(NUM_TARGET_TYPES, false);
};

using Spells = std::map < Spell::ID , Spell * >;
