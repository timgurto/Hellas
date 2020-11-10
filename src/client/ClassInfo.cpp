#include "ClassInfo.h"

#include "../Message.h"
#include "Client.h"
#include "Tooltip.h"

ClassInfo::ClassInfo(const Name &name) : _name(name), _trees{} {
  _image = {"Images/Humans/" + name};
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

const Tooltip &ClientTalent::tooltip(const Client &client) const {
  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();

  tooltip.setColor(Color::TOOLTIP_NAME);
  tooltip.addLine(name);
  tooltip.addGap();

  tooltip.setColor(Color::TOOLTIP_BODY);
  if (hasCost()) {
    auto tagName = client.gameData.tagName(costTag);
    tooltip.addLine("Costs "s + toString(costQuantity) + " "s + tagName +
                    " to learn"s);
  }
  if (reqPointsInTree > 0)
    tooltip.addLine("Requires "s + toString(reqPointsInTree) + " points in "s +
                    tree);
  if (!reqTool.empty())
    tooltip.addLine("Requires "s + client.gameData.tagName(reqTool) +
                    " building"s);
  if (hasCost() || reqPointsInTree > 0 || !reqTool.empty()) tooltip.addGap();

  if (!flavourText.empty()) {
    tooltip.setColor(Color::TOOLTIP_FLAVOUR);
    tooltip.addLine(flavourText);
    tooltip.addGap();
  }

  switch (type) {
    case SPELL:
      tooltip.setColor(Color::TOOLTIP_BODY);
      tooltip.addLine("Teaches you this ability:");
      tooltip.embed(spell->tooltip());
      break;
    case STATS:
      tooltip.setColor(Color::TOOLTIP_BODY);
      tooltip.addLine("Each point grants you:");
      tooltip.addLines(stats.toStrings());
      break;

    default:
      assert(false);
  }

  return tooltip;
}

void ClientTalent::generateLearnMessage() {
  learnMessage = {CL_CHOOSE_TALENT, name};
}

size_t Tree::numTiers() const {
  auto highestTier = size_t{0};
  for (auto pair : talents)
    if (pair.first > highestTier) highestTier = pair.first;
  return highestTier + 1;  // Assuming lowest is 0
}
