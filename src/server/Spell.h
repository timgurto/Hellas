#pragma once

#include <map>
#include <vector>

#include "Entity.h"
#include "../Podes.h"
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

    void canTarget(TargetType type) { _validTargets[type] = true; }
    void cost(Energy e) { _cost = e; }
    void range(Podes r) { _range = r.toPixels(); }

private:
    using Args = std::vector<int>;
    using Function = Outcome(*)(Entity &caster, Entity &target, const Args &args);

    Function _function = nullptr;
    Args _args = {};

    using FunctionMap = std::map<std::string, Function>;
    static FunctionMap functionMap;

    static Outcome doDirectDamage(Entity &caster, Entity &target, const Args &args);
    static Outcome heal(Entity &caster, Entity &target, const Args &args);

    Energy _cost = 0;
    px_t _range = 0;

    using ValidTargets = std::vector<bool>;
    ValidTargets _validTargets = ValidTargets(NUM_TARGET_TYPES, false);
};

using Spells = std::map<std::string, Spell *>;
