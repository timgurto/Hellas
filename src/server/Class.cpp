#include "Class.h"

Class::Class(const ClassType *type) :
    _type(type)
{}

bool Class::knowsSpell(const Spell::ID & spell) const {
    for (const auto talent : _takenTalents) {
        if (talent->type() != Talent::SPELL)
            continue;
        if (talent->spellID() == spell)
            return true;
    }
    return false;
}

std::string Class::generateKnownSpellsString() const {
    auto string = ""s;
    for (auto talent : _takenTalents) {
        if (talent->type() != Talent::SPELL)
            continue;
        if (!string.empty())
            string.append(",");
        string.append(talent->spellID());
    }
    return string;
}

Talent Talent::Dummy(const Name & name) {
    return{ name, DUMMY };
}

Talent Talent::Spell(const Name &name, const Spell::ID &id) {
    auto t = Talent{ name, SPELL };
    t._spellID = id;
    return t;
}

bool Talent::operator<(const Talent & rhs) const {
    return _name < rhs._name;
}

Talent::Talent(const Name & name, Type type):
_name(name),
_type(type)
{}

void ClassType::addSpell(const Talent::Name & name, Spell::ID & spellID) {
    _talents.insert(Talent::Spell(name, spellID));
}

const Talent * ClassType::findTalent(const Talent::Name &name) const {
    auto it = _talents.find(Talent::Dummy(name));
    if (it == _talents.end())
        return nullptr;
    return &*it;
}
