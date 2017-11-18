#pragma once

#include "Spell.h"

using LearnedSpells = std::map<Spell::ID, bool>;

class ClassType {
public:
    using ID = std::string;

    ClassType(const ID &id = {}) : _id(id) {}

    const ID &id() const { return _id; }

    void addSpell(const Spell::ID &id) { _spells.insert(id); }
    LearnedSpells && instantiateLearnedSpellsList() const;

private:
    ID _id;
    std::set<Spell::ID> _spells;
};

using ClassTypes = std::map<ClassType::ID, ClassType>;


// A single user's instance of a ClassType
class Class {
public:

    Class(const ClassType &type);

private:
    const ClassType &_type;
    LearnedSpells _learnedSpells;
};
