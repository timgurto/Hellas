#include <cassert>

#include "Class.h"
#include "User.h"

bool Class::canTakeATalent() const {
    return _talentPointsAllocated < talentPointsAvailable();
}

int Class::talentPointsAvailable() const {
    return _owner->level() + 1;
}

Class::Class(const ClassType &type, const User &owner) :
    _type(&type),
    _owner(&owner)
{}

void Class::takeTalent(const Talent * talent) {
    ++_talentPointsAllocated;

    if (_talentRanks.find(talent) == _talentRanks.end()) {
        _talentRanks[talent] = 1;
        return;
    }
    assert(talent->type() == Talent::STATS || _talentRanks[talent] == 0);
    ++_talentRanks[talent];
}

bool Class::knowsSpell(const Spell::ID & spell) const {
    for (const auto pair : _talentRanks) {
        auto talent = pair.first;
        if (talent->type() != Talent::SPELL)
            continue;
        if (talent->spellID() != spell)
            continue;
        return pair.second > 0;
    }
    return false;
}

std::string Class::generateKnownSpellsString() const {
    auto string = ""s;
    for (auto pair : _talentRanks) {
        if (pair.second == 0)
            continue;
        auto talent = pair.first;
        if (talent->type() != Talent::SPELL)
            continue;
        if (!string.empty())
            string.append(",");
        string.append(talent->spellID());
    }
    return string;
}

void Class::applyStatsTo(Stats &baseStats) const {
    for (auto pair : _talentRanks) {
        auto talent = pair.first;
        if (talent->type() != Talent::STATS)
            continue;

        auto rank = pair.second;
        for (auto i = 0; i != rank; ++i)
            baseStats &= talent->stats();
    }
}

Talent Talent::Dummy(const Name & name) {
    return{ name, DUMMY };
}

Talent Talent::Spell(const Name &name, const Spell::ID &id) {
    auto t = Talent{ name, SPELL };
    t._spellID = id;
    return t;
}

Talent Talent::Stats(const Name & name, const StatsMod & stats) {
    auto t = Talent{ name, STATS };
    t._stats = stats;
    return t;
}

bool Talent::operator<(const Talent & rhs) const {
    return _name < rhs._name;
}

Talent::Talent(const Name & name, Type type):
_name(name),
_type(type)
{}

void ClassType::addSpell(const Talent::Name & name, const Spell::ID & spellID) {
    _talents.insert(Talent::Spell(name, spellID));
}

void ClassType::addStats(const Talent::Name & name, const StatsMod & stats) {
    _talents.insert(Talent::Stats(name, stats));
}

const Talent * ClassType::findTalent(const Talent::Name &name) const {
    auto it = _talents.find(Talent::Dummy(name));
    if (it == _talents.end())
        return nullptr;
    return &*it;
}
