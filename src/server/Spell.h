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

    void setFunction(const std::string &functionName);
    void addArg(int arg) { _args.push_back(arg); }

    Outcome performAction(Entity &caster, Entity &target) const;

private:
    using Args = std::vector<int>;
    using Function = Outcome(*)(Entity &caster, Entity &target, const Args &args);

    Function _function = nullptr;
    Args _args = {};

    using FunctionMap = std::map<std::string, Function>;
    static FunctionMap functionMap;

    static Outcome doDirectDamage(Entity &caster, Entity &target, const Args &args);
};

using Spells = std::map<std::string, Spell *>;
