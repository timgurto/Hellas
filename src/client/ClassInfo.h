#pragma once

#include <map>
#include <set>
#include <string>

#include "ClientSpell.h"
#include "Texture.h"
#include "../Stats.h"

struct ClientTalent {
    enum Type {
        SPELL,
        STATS,

        UNINITIALIZED
    };

    using Name = std::string;
    using Tree = std::string;

    ClientTalent(const Name &talentName, const Tree &treeName, Type type);

    bool operator< (const ClientTalent &rhs) const { return name < rhs.name; }

    Name name{};
    Tree tree{};
    Type type{ UNINITIALIZED };
    std::string learnMessage{};
    const Texture *icon{ nullptr };

    const ClientSpell *spell{ nullptr };
    StatsMod stats{};
};

class ClassInfo {
public:
    using Name = std::string;
    using Container = std::map<Name, ClassInfo>;
    using Talents = std::set<ClientTalent>;
    using Trees = std::set<Name>;

    ClassInfo() {}
    ClassInfo(const Name &name);

    void addSpell(const ClientTalent::Name &talentName, const ClientTalent::Tree &tree, const ClientSpell *spell);
    void addStats(const ClientTalent::Name &talentName, const ClientTalent::Tree &tree, const StatsMod &stats);
    void addTree(const Name &name) { _trees.insert(name); }

    const Name &name() const { return _name; }
    const Texture &image() const { return _image; }
    const Talents &talents() const { return _talents; }
    const Trees &trees() const { return _trees; }

private:
    Name _name{};
    Texture _image{};
    Talents _talents{};
    Trees _trees{};
};
