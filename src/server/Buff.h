#pragma once

#include <map>
#include <string>
#include <vector>

#include "../Stats.h"


class BuffType {
public:
    BuffType() {}

    void stats(const StatsMod &stats) { _stats = stats; }
    const StatsMod &stats() const { return _stats; }

private:
    StatsMod _stats = {};
};

class Buff {
public:
    using ID = std::string;

    Buff(const BuffType &type) : _type(type) {}
    void applyTo(Stats &stats) { stats &= _type.stats(); }

private:
    const BuffType &_type;
};


using BuffTypes = std::map<Buff::ID, BuffType>;
using Buffs = std::vector<Buff>;
