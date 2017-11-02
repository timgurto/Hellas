#pragma once

#include <map>
#include <vector>

#include "Entity.h"
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

    void setFunction(const std::string &functionName);
    void addArg(int arg) { _args.push_back(arg); }

    CombatResult performAction(Entity &caster, Entity &target) const;
        bool isTargetValid(const Entity &caster, const Entity &target) const;

    void canTarget(TargetType type) { _validTargets[type] = true; }
    void cost(Energy e) { _cost = e; }
    Energy cost() const { return _cost; }
    void range(Podes r) { _range = r.toPixels(); }
    void radius(Podes r) { _range = r.toPixels(); _targetsInArea = true; }
    px_t range() const { return _range; }
    bool isAoE() const { return _targetsInArea; }
    void school(SpellSchool school) { _school = school; }
    bool shouldPlayDefenseSound() const { return aggressionMap[_function]; }
    static Hitpoints chooseRandomSpellMagnitude(double raw);

private:
    using Args = std::vector<int>;
    using Function = CombatResult(*)(Entity &caster, Entity &target, const Args &args);

    Function _function = nullptr;
    Args _args = {};

    using FunctionMap = std::map<std::string, Function>;
    static FunctionMap functionMap;
    using FlagMap = std::map<Function, bool>;
    static FlagMap aggressionMap; // Ultimately, whether a defense sound should play.

    static CombatResult doDirectDamage(Entity &caster, Entity &target, const Args &args);
    static CombatResult heal(Entity &caster, Entity &target, const Args &args);

    Energy _cost = 0;
    px_t _range = Podes::MELEE_RANGE.toPixels();
    bool _targetsInArea = false;
    SpellSchool _school;

    using ValidTargets = std::vector<bool>;
    ValidTargets _validTargets = ValidTargets(NUM_TARGET_TYPES, false);
};

using Spells = std::map<std::string, Spell *>;
