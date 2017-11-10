#pragma once

#include <map>
#include <set>
#include <string>

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

    bool operator<(const Buff &rhs) const { return &_type < &rhs._type; }

    void applyTo(Stats &stats) const { stats &= _type.stats(); }

private:
    const BuffType &_type;
};

using BuffTypes = std::map<Buff::ID, BuffType>;
using Buffs = std::set<Buff>;
