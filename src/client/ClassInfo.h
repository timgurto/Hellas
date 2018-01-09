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

    bool operator< (const ClientTalent &rhs) const { return name < rhs.name; }

    Name name{};
    Type type{ UNINITIALIZED };
    std::shared_ptr<std::string> learnMessage{};
    Texture icon{};
    const Texture tooltip() const;

    void generateLearnMessage();

    const ClientSpell *spell{ nullptr };
    StatsMod stats{};
    std::string flavourText{};

    std::string costTag{};
    size_t costQuantity{ 0 };
    size_t reqPointsInTree{ 0 };
    std::string tree;
    bool hasCost() const { return !costTag.empty() && costQuantity > 0; }
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

    void addTalentToTree(const ClientTalent &talent, const Tree::Name &tree, Tree::Tier tier);
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
