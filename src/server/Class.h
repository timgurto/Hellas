#pragma once

#include "Spell.h"

class ClassType {
public:
    using ID = std::string;

    ClassType(const ID &id = {}) : _id(id) {}

    void addSpell(const Spell::ID &id) { _spells.insert(id); }

private:
    ID _id;
    std::set<Spell::ID> _spells;
};

using ClassTypes = std::map<ClassType::ID, ClassType>;
