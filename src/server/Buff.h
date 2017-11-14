#pragma once

#include <map>
#include <set>
#include <string>

#include "../Stats.h"


class BuffType {
public:
    using ID = std::string;

    BuffType() {}
    BuffType(const ID &id) : _id(id) {}

    void stats(const StatsMod &stats) { _stats = stats; }
    const StatsMod &stats() const { return _stats; }
    const ID &id() const { return _id; }

private:
    StatsMod _stats{};
    ID _id{};
};

class Buff {
public:
    using ID = std::string;

    Buff(const BuffType &type) : _type(type) {}

    const ID &type() const { return _type.id(); }

    bool operator<(const Buff &rhs) const { return &_type < &rhs._type; }

    void applyTo(Stats &stats) const { stats &= _type.stats(); }

private:
    const BuffType &_type;
};

using BuffTypes = std::map<Buff::ID, BuffType>;
using Buffs = std::set<Buff>;
