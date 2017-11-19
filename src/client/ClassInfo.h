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

    ClassInfo() {}
    ClassInfo(const Name &name);

    void addSpell(const ClientSpell *spell) { _spells.insert(spell); }

    const Name &name() const { return _name; }
    const Texture &image() const { return _image; }
    const Spells &spells() const { return _spells; }

private:
    Name _name;
    Texture _image;
    Spells _spells;
};
