#include <cassert>

#include "Class.h"
#include "Server.h"
#include "User.h"

const Tier Talent::DUMMY_TIER;

bool Class::canTakeATalent() const {
    return _talentPointsAllocated < talentPointsAvailable();
}

int Class::talentPointsAvailable() const {
    return _owner->level() - 1;
}

void Class::unlearnAll() {
    for (auto &pair : _talentRanks) {
        pair.second = 0;
    }
    _talentPointsAllocated = 0;

    const auto &server = Server::instance();
    server.sendMessage(_owner->socket(), SV_NO_TALENTS);
}

Class::Class(const ClassType &type, const User &owner) :
    _type(&type),
    _owner(&owner)
{}

void Class::takeTalent(const Talent * talent) {
    ++_talentPointsAllocated;

    if (_talentRanks.find(talent) == _talentRanks.end()) {
        _talentRanks[talent] = 1;
    } else {
        assert(talent->type() == Talent::STATS || _talentRanks[talent] == 0);
        ++_talentRanks[talent];
    }

    const auto &server = Server::instance();
    server.sendMessage(_owner->socket(), SV_TALENT, makeArgs(talent->name(), _talentRanks[talent]));
    server.sendMessage(_owner->socket(), SV_POINTS_IN_TREE,
        makeArgs(talent->tree(), pointsInTree(talent->tree())));
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
    auto spellsKnown = 0;
    for (auto pair : _talentRanks) {
        if (pair.second == 0)
            continue;
        auto talent = pair.first;
        if (talent->type() != Talent::SPELL)
            continue;
        string.append(std::string{ MSG_DELIM });
        string.append(talent->spellID());
        ++spellsKnown;
    }
    return toString(spellsKnown) + string;
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

size_t Class::pointsInTree(const std::string & treeName) const {
    auto total = 0;
    for (auto pair : _talentRanks) {
        if (pair.first->tree() == treeName)
            total += pair.second;
    }
    return total;
}

void Class::loadTalentRank(const Talent & talent, unsigned rank) {
    _talentPointsAllocated += rank;
    _talentRanks[&talent] = rank;
}

Talent::Name Class::loseARandomLeafTalent() {
    auto candidates = std::vector<const Talent*>{};
    for (const auto &pair : _talentRanks) {
        if (isLeafTalent(*pair.first))
            candidates.push_back(pair.first);
    }

    if (candidates.empty())
        return{};

    auto indexToDrop = rand() % candidates.size();
    auto talentToDrop = candidates[indexToDrop];

    --_talentRanks[talentToDrop];
    --_talentPointsAllocated;

    const auto &server = Server::instance();
    server.sendMessage(_owner->socket(), SV_TALENT,
        makeArgs(talentToDrop->name(), _talentRanks[talentToDrop]));
    server.sendMessage(_owner->socket(), SV_POINTS_IN_TREE,
        makeArgs(talentToDrop->tree(), pointsInTree(talentToDrop->tree())));

    return talentToDrop->name();
}

bool Class::isLeafTalent(const Talent & talent) {
    // Not a talent --> not a leaf talent
    if (_talentRanks[&talent] == 0)
        return false;

    auto withoutTalent = _talentRanks;
    --withoutTalent[&talent];
    return (allTalentsAreSupported(withoutTalent));
}

bool Class::allTalentsAreSupported(TalentRanks &talentRanks) {
    for (const auto &pair : talentRanks) {
        if (!talentIsSupported(pair.first, talentRanks))
            return false;
    }
    return true;
}

bool Class::talentIsSupported(const Talent * talent, TalentRanks & talentRanks) {
    // Talent isn't taken, and so doesn't need support.
    auto rank = talentRanks[talent];
    if (rank == 0)
        return true;

    auto pointsRequired = talent->tier().reqPointsInTree;

    // Talent has no point req; i.e., no need for support.
    if (pointsRequired == 0)
        return true;

    for (const auto &supportingTalent : talentRanks) {
        // 0 points = no support
        if (supportingTalent.second == 0)
            continue;

        // This talent is more advanced, and so can't support it.
        if (supportingTalent.first->tier().reqPointsInTree > talent->tier().reqPointsInTree)
            continue;

        // A talent can't support itself
        if (supportingTalent.first == talent)
            continue;

        if (supportingTalent.second >= pointsRequired)
            return true;
        pointsRequired -= supportingTalent.second;
    }

    return false;
}

Talent Talent::Dummy(const Name & name) {
    return{ name, DUMMY , DUMMY_TIER };
}

Talent Talent::Spell(const Name &name, const Spell::ID &id, const Tier &tier) {
    auto t = Talent{ name, SPELL, tier };
    t._spellID = id;
    return t;
}

Talent Talent::Stats(const Name & name, const StatsMod & stats, const Tier &tier) {
    auto t = Talent{ name, STATS, tier };
    t._stats = stats;
    return t;
}

bool Talent::operator<(const Talent & rhs) const {
    return _name < rhs._name;
}

Talent::Talent(const Name & name, Type type, const Tier &tier):
_name(name),
_type(type),
_tier(tier)
{}

Talent &ClassType::addSpell(const Talent::Name & name, const Spell::ID & spellID, const Tier &tier) {
    auto pair = _talents.insert(Talent::Spell(name, spellID, tier));
    return const_cast<Talent &>(*pair.first);
}

Talent &ClassType::addStats(const Talent::Name & name, const StatsMod & stats, const Tier &tier) {
    auto pair = _talents.insert(Talent::Stats(name, stats, tier));
    return const_cast<Talent &>(*pair.first);
}

const Talent * ClassType::findTalent(const Talent::Name &name) const {
    auto it = _talents.find(Talent::Dummy(name));
    if (it == _talents.end())
        return nullptr;
    return &*it;
}
