#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include "../Message.h"
#include "../Stats.h"
#include "ClientSpell.h"
#include "Texture.h"

class Element;

struct ClientTalent {
  enum Type {
    SPELL,
    STATS,

    UNINITIALIZED
  };

  using Name = std::string;

  bool operator<(const ClientTalent &rhs) const { return name < rhs.name; }

  Name name{};
  Type type{UNINITIALIZED};
  Message learnMessage{};
  Texture icon{};
  mutable Optional<Tooltip> _tooltip;
  const Tooltip &tooltip(const Client &client) const;

  void generateLearnMessage();

  const ClientSpell *spell{nullptr};
  StatsMod stats{};
  std::string flavourText{};

  std::string costTag{};
  size_t costQuantity{0};
  size_t reqPointsInTree{0};
  std::string reqTool{};
  std::string tree;
  bool hasCost() const { return !costTag.empty() && costQuantity > 0; }
};

struct Tree {
  using Name = std::string;
  using Tier = unsigned;
  using TalentsByTier = std::map<Tier, std::vector<ClientTalent> >;

  size_t numTiers() const;

  Name name{};
  mutable Element *element{nullptr};
  TalentsByTier talents{};
};

class ClassInfo {
 public:
  using Name = std::string;
  using Container = std::map<Name, ClassInfo>;
  using Trees = std::vector<Tree>;
  using Description = std::string;

  ClassInfo() {}
  ClassInfo(const Name &name);

  void addTalentToTree(const ClientTalent &talent, const Tree::Name &tree,
                       Tree::Tier tier);
  void ensureTreeExists(const Tree::Name &name);
  Tree &findTree(const Tree::Name &name);

  const Name &name() const { return _name; }
  const Texture &image() const { return _image.getNormalImage(); }
  const Texture &highlightImage() const { return _image.getHighlightImage(); }
  const Trees &trees() const { return _trees; }
  const Description &description() const { return _description; }
  void description(const Description &desc) { _description = desc; }

 private:
  Name _name{};
  ImageWithHighlight _image{};
  Trees _trees{};
  Description _description{};
};
