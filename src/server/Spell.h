#pragma once

#include <map>
#include <vector>

#include "Entity.h"
#include "../types.h"

class Spell {
public:
    enum Outcome {
        FAIL,

        HIT,
        CRIT,
        MISS
    };

    enum TargetType {
        SELF,
        FRIENDLY,
        ENEMY,

        NUM_TARGET_TYPES
    };

    void setFunction(const std::string &functionName);
    void addArg(int arg) { _args.push_back(arg); }

    Outcome performAction(Entity &caster, Entity &target) const;
        bool isTargetValid(const Entity &caster, const Entity &target) const;

    void setCanTarget(TargetType type) { _validTargets[type] = true; }

private:
    using Args = std::vector<int>;
    using Function = Outcome(*)(Entity &caster, Entity &target, const Args &args);

    Function _function = nullptr;
    Args _args = {};

    using FunctionMap = std::map<std::string, Function>;
    static FunctionMap functionMap;

    using ValidTargets = std::vector<bool>;
    ValidTargets _validTargets = ValidTargets(NUM_TARGET_TYPES, false);

    static Outcome doDirectDamage(Entity &caster, Entity &target, const Args &args);
    static Outcome heal(Entity &caster, Entity &target, const Args &args);
};

using Spells = std::map<std::string, Spell *>;
