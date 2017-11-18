#pragma once

#include "Spell.h"

using LearnedSpells = std::set<Spell::ID>;

class ClassType {
public:
    using ID = std::string;

    ClassType(const ID &id = {}) : _id(id) {}

    const ID &id() const { return _id; }
    bool isValidSpell(const ID &id) const { return _spells.find(id) != _spells.end(); }

    void addSpell(const Spell::ID &id) { _spells.insert(id); }

private:
    ID _id;
    std::set<Spell::ID> _spells;
};

using ClassTypes = std::map<ClassType::ID, ClassType>;


// A single user's instance of a ClassType
class Class {
public:

    Class(const ClassType *type = nullptr);
    const ClassType &type() const { assert(_type);  return *_type; }

    bool knowsSpell(const Spell::ID &id) const { return _learnedSpells.find(id) != _learnedSpells.end(); }
    void learnSpell(const Spell::ID &id) { _learnedSpells.insert(id); }
    std::string generateKnownSpellsString() const;

private:
    const ClassType *_type = nullptr;
    LearnedSpells _learnedSpells{};
};
