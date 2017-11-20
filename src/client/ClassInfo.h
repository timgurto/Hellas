#pragma once

#include <map>
#include <set>
#include <string>

#include "Texture.h"

class ClientSpell;

class ClassInfo {
public:
    using Name = std::string;
    using Container = std::map<Name, ClassInfo>;
    using Spells = std::set<const ClientSpell *>;
    using Trees = std::set<Name>;

    ClassInfo() {}
    ClassInfo(const Name &name);

    void addSpell(const ClientSpell *spell) { _spells.insert(spell); }
    void addTree(const Name &name) {
        _trees.insert(name);
    }

    const Name &name() const { return _name; }
    const Texture &image() const { return _image; }
    const Spells &spells() const { return _spells; }
    const Trees &trees() const { return _trees; }

private:
    Name _name{};
    Texture _image{};
    Spells _spells{};
    Trees _trees{};
};
