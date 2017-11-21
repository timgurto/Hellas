#pragma once

#include "Spell.h"

class Talent {
public:
    using Name = std::string;

    enum Type {
        SPELL,

        DUMMY
    };

    static Talent Dummy(const Name &name);
    static Talent Spell(const Name &name, const Spell::ID &id);

    bool operator<(const Talent &rhs) const;

    Type type() const { return _type; }
    const Spell::ID &spellID() const { return _spellID; }

private:

    Talent(const Name &name, Type type);

    Name _name;
    Type _type;
    Spell::ID _spellID;
};

class ClassType {
public:
    using ID = std::string;

    ClassType(const ID &id = {}) : _id(id) {}

    const ID &id() const { return _id; }

    void addSpell(const Talent::Name &name, Spell::ID &spellID);
    const Talent *findTalent(const Talent::Name &name) const;

private:
    ID _id;
    std::set<Talent> _talents;
};

using ClassTypes = std::map<ClassType::ID, ClassType>;

// A single user's instance of a ClassType
class Class {
public:
    using TakenTalents = std::set<const Talent *>;

    Class(const ClassType *type = nullptr);
    const ClassType &type() const { assert(_type);  return *_type; }

    bool hasTalent(const Talent *talent) const { return _takenTalents.find(talent) != _takenTalents.end(); }
    void takeTalent(const Talent *talent) { _takenTalents.insert(talent); }
    bool knowsSpell(const Spell::ID &spell) const;
    std::string generateKnownSpellsString() const;

private:
    const ClassType *_type = nullptr;
    TakenTalents _takenTalents{};
};
