#include "ClassInfo.h"
#include "Client.h"
#include "Tooltip.h"

ClassInfo::ClassInfo(const Name &name) : _name(name), _trees{} {
    _image = { "Images/Humans/" + name + ".png", Color::MAGENTA };
}

void ClassInfo::addTalentToTree(const ClientTalent & talent, const Tree::Name & treeName,
    Tree::Tier tier) {
    auto &tree = findTree(treeName);
    tree.talents[tier].push_back(talent);
}

void ClassInfo::ensureTreeExists(const Name & name) {
    for (const auto &tree : _trees) {
        if (tree.name == name)
            return;
    }
    auto tree = Tree{};
    tree.name = name;
    _trees.push_back(tree);
}

Tree & ClassInfo::findTree(const Tree::Name & name) {
    for (auto &tree : _trees) {
        if (tree.name == name)
            return tree;
    }
    assert(false);
    return _trees.front();
}

const Tooltip &ClientTalent::tooltip() const {
    if (_tooltip.hasValue())
        return _tooltip.value();

    _tooltip = Tooltip{};
    auto &tooltip = _tooltip.value();

    tooltip.setColor(Color::ITEM_NAME);
    tooltip.addLine(name);
    tooltip.addGap();

    tooltip.setColor(Color::ITEM_TAGS);
    if (hasCost()) {
        auto tagName = Client::instance().tagName(costTag);
        tooltip.addLine("Costs "s + tagName + " x"s + toString(costQuantity));
    }
    if (reqPointsInTree > 0)
        tooltip.addLine("Requires "s + toString(reqPointsInTree) + " points in "s + tree);
    if (hasCost() || reqPointsInTree > 0)
        tooltip.addGap();

    if (!flavourText.empty()) {
        tooltip.setColor(Color::FLAVOUR_TEXT);
        tooltip.addLine(flavourText);
        tooltip.addGap();
    }

    switch (type) {
    case SPELL:
        break;
    case STATS:
        tooltip.setColor(Color::ITEM_STATS);
        tooltip.addLine("Each level:");
        tooltip.addLines(stats.toStrings());
        break;

    default:
        assert(false);
    }

        return tooltip;
}

void ClientTalent::generateLearnMessage() {
    auto &client = Client::instance();
    learnMessage = std::make_shared<std::string>(Client::compileMessage(CL_TAKE_TALENT, name));
}

size_t Tree::numTiers() const {
    auto highestTier = size_t{ 0 };
    for (auto pair : talents)
        if (pair.first > highestTier)
            highestTier = pair.first;
    return highestTier + 1; // Assuming lowest is 0
}
