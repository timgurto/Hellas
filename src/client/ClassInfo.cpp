#include "ClassInfo.h"
#include "Client.h"
#include "Tooltip.h"

ClassInfo::ClassInfo(const Name &name) : _name(name), _trees{} {
  _image = {"Images/Humans/" + name + ".png", Color::MAGENTA};
}

void ClassInfo::addTalentToTree(const ClientTalent &talent,
                                const Tree::Name &treeName, Tree::Tier tier) {
  auto &tree = findTree(treeName);
  tree.talents[tier].push_back(talent);
}

void ClassInfo::ensureTreeExists(const Name &name) {
  for (const auto &tree : _trees) {
    if (tree.name == name) return;
  }
  auto tree = Tree{};
  tree.name = name;
  _trees.push_back(tree);
}

Tree &ClassInfo::findTree(const Tree::Name &name) {
  for (auto &tree : _trees) {
    if (tree.name == name) return tree;
  }
  assert(false);
  return _trees.front();
}

const Tooltip &ClientTalent::tooltip() const {
  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();

  tooltip.setColor(Color::TODO);
  tooltip.addLine(name);
  tooltip.addGap();

  tooltip.setColor(Color::TODO);
  if (hasCost()) {
    auto tagName = Client::instance().tagName(costTag);
    tooltip.addLine("Costs "s + toString(costQuantity) + " "s + tagName);
  }
  if (reqPointsInTree > 0)
    tooltip.addLine("Requires "s + toString(reqPointsInTree) + " points in "s +
                    tree);
  if (hasCost() || reqPointsInTree > 0) tooltip.addGap();

  if (!flavourText.empty()) {
    tooltip.setColor(Color::TODO);
    tooltip.addLine(flavourText);
    tooltip.addGap();
  }

  switch (type) {
    case SPELL:
      tooltip.setColor(Color::TODO);
      tooltip.addLine("Teaches you this ability:");
      tooltip.embed(spell->tooltip());
      break;
    case STATS:
      tooltip.setColor(Color::TODO);
      tooltip.addLine("Each point grants you:");
      tooltip.addLines(stats.toStrings());
      break;

    default:
      assert(false);
  }

  return tooltip;
}

void ClientTalent::generateLearnMessage() {
  auto &client = Client::instance();
  learnMessage = Client::compileMessage(CL_TAKE_TALENT, name);
}

size_t Tree::numTiers() const {
  auto highestTier = size_t{0};
  for (auto pair : talents)
    if (pair.first > highestTier) highestTier = pair.first;
  return highestTier + 1;  // Assuming lowest is 0
}
