#pragma once

#include <list>

#include "Spell.h"

// A single tier of a single tree.  3 class * 3 trees * 5 tiers = 45 Tiers total
struct Tier {
    std::string costTag;
    size_t costQuantity{ 0 };

    bool hasItemCost() const { return !costTag.empty() && costQuantity > 0; }
};
using Tiers = std::list<Tier>;

class Talent {
public:
    using Name = std::string;

    enum Type {
        SPELL,
        STATS,

        DUMMY
    };

    static Talent Dummy(const Name &name);
    static Talent Spell(const Name &name, const Spell::ID &id, const Tier &tier);
    static Talent Stats(const Name &name, const StatsMod &stats, const Tier &tier);

    bool operator<(const Talent &rhs) const;

    Type type() const { return _type; }
    const Spell::ID &spellID() const { return _spellID; }
    const StatsMod &stats() const { return _stats; }
    const Tier &tier() const { return _tier; }

private:

    Talent(const Name & name, Type type, const Tier &tier);

    Name _name{};
    Type _type{ DUMMY };
    Spell::ID _spellID{};
    StatsMod _stats{};
    const Tier &_tier;

    static const Tier DUMMY_TIER;
};

class ClassType {
public:
    using ID = std::string;

    ClassType(const ID &id = {}) : _id(id) {}

    const ID &id() const { return _id; }

    void addSpell(const Talent::Name &name, const Spell::ID &spellID, const Tier &tier);
    void addStats(const Talent::Name &name, const StatsMod &stats, const Tier &tier);
    const Talent *findTalent(const Talent::Name &name) const;

private:
    ID _id;
    std::set<Talent> _talents;
};

using ClassTypes = std::map<ClassType::ID, ClassType>;

class User;

// A single user's instance of a ClassType
class Class {
public:
    using TalentRanks = std::map<const Talent *, unsigned>;
    bool canTakeATalent() const;
    int talentPointsAvailable() const;

    Class() = default;
    Class(const ClassType &type, const User &owner);
    const ClassType &type() const { return *_type; }

    bool hasTalent(const Talent *talent) { return _talentRanks[talent] > 0; }
    void takeTalent(const Talent *talent);
    bool knowsSpell(const Spell::ID &spell) const;
    std::string generateKnownSpellsString() const;
    void applyStatsTo(Stats &baseStats) const;

private:
    const ClassType *_type{ nullptr };
    TalentRanks _talentRanks{};
    int _talentPointsAllocated{ 0 }; // Updated in takeTalent()
    const User *_owner{ nullptr };
};
