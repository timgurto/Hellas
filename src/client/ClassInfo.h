#pragma once

#include <map>
#include <set>
#include <string>

#include "Texture.h"

class ClientSpell;

class ClassInfo {
public:
    using ID = std::string;
    using Name = std::string;
    using Container = std::map<ID, ClassInfo>;
    using Spells = std::set<const ClientSpell *>;

    ClassInfo() {}
    ClassInfo(const ID &id, const Name &name);

    void addSpell(const ClientSpell *spell) { _spells.insert(spell); }

    const Name &name() const { return _name; }
    const Texture &image() const { return _image; }
    const Spells &spells() const { return _spells; }

private:
    Name _name;
    Texture _image;
    Spells _spells;
};
