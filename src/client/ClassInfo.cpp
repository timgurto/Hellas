#include "ClassInfo.h"
#include "Client.h"
#include "TooltipBuilder.h"

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

const Texture ClientTalent::tooltip() const {
    switch (type) {
    case SPELL:
        return spell->tooltip();
    case STATS:
    {
        auto tb = Tooltip{};
        tb.setColor(Color::ITEM_NAME);
        tb.addLine(name);
        tb.addGap();

        if (!flavourText.empty()) {
            tb.setColor(Color::FLAVOUR_TEXT);
            tb.addLine(flavourText);
            tb.addGap();
        }

        tb.setColor(Color::ITEM_STATS);
        tb.addLine("Each level:");
        tb.addLines(stats.toStrings());

        return tb.publish();
    }

    default:
        assert(false);
        return {};
    }
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
