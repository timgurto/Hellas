#pragma once

#include "Spell.h"

class Talent {
public:
    using Name = std::string;

    enum Type {
        SPELL,
        STATS,

        DUMMY
    };

    static Talent Dummy(const Name &name);
    static Talent Spell(const Name &name, const Spell::ID &id);
    static Talent Stats(const Name &name, const StatsMod &stats);

    bool operator<(const Talent &rhs) const;

    Type type() const { return _type; }
    const Spell::ID &spellID() const { return _spellID; }
    const StatsMod &stats() const { return _stats; }

private:

    Talent(const Name &name, Type type);

    Name _name{};
    Type _type{ DUMMY };
    Spell::ID _spellID{};
    StatsMod _stats{};
};

class ClassType {
public:
    using ID = std::string;

    ClassType(const ID &id = {}) : _id(id) {}

    const ID &id() const { return _id; }

    void addSpell(const Talent::Name &name, const Spell::ID &spellID);
    void addStats(const Talent::Name &name, const StatsMod &stats);
    const Talent *findTalent(const Talent::Name &name) const;

private:
    ID _id;
    std::set<Talent> _talents;
};

using ClassTypes = std::map<ClassType::ID, ClassType>;

// A single user's instance of a ClassType
class Class {
public:
    using TalentRanks = std::map<const Talent *, unsigned>;

    Class(const ClassType *type = nullptr);
    const ClassType &type() const { assert(_type);  return *_type; }

    bool hasTalent(const Talent *talent) { return _talentRanks[talent] > 0; }
    void takeTalent(const Talent *talent);
    bool knowsSpell(const Spell::ID &spell) const;
    std::string generateKnownSpellsString() const;
    void applyStatsTo(Stats &baseStats) const;

private:
    const ClassType *_type = nullptr;
    TalentRanks _talentRanks{};
};
