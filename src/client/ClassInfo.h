#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include "ClientSpell.h"
#include "Texture.h"
#include "../Stats.h"

class Element;

struct ClientTalent {
    enum Type {
        SPELL,
        STATS,

        UNINITIALIZED
    };

    using Name = std::string;

    ClientTalent(const Name &talentName, Type type);

    bool operator< (const ClientTalent &rhs) const { return name < rhs.name; }

    Name name{};
    Name tree{};
    unsigned tier{ 0 };
    Type type{ UNINITIALIZED };
    std::shared_ptr<std::string> learnMessage{};
    const Texture *icon{ nullptr };
    const Texture tooltip() const;

    const ClientSpell *spell{ nullptr };
    StatsMod stats{};
};

struct Tree {
    using Name = std::string;
    using Tier = unsigned;
    using TalentsByTier = std::map<Tier, std::vector<ClientTalent> >;

    size_t numTiers() const;
    
    Name name{};
    mutable Element *element{ nullptr };
    TalentsByTier talents{};
};

class ClassInfo {
public:
    using Name = std::string;
    using Container = std::map<Name, ClassInfo>;
    using Trees = std::vector<Tree>;

    ClassInfo() {}
    ClassInfo(const Name &name);

    void addSpell(const ClientTalent::Name &talentName, const Tree::Name &tree, unsigned tier, const ClientSpell *spell);
    void addStats(const ClientTalent::Name &talentName, const Tree::Name &tree, unsigned tier, const StatsMod &stats);
    void ensureTreeExists(const Tree::Name &name);
    Tree &findTree(const Tree::Name &name);

    const Name &name() const { return _name; }
    const Texture &image() const { return _image; }
    const Trees &trees() const { return _trees; }

private:
    Name _name{};
    Texture _image{};
    Trees _trees{};
};
