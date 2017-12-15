#include "Client.h"
#include "ui/Line.h"
#include "ui/ProgressBar.h"

void Client::initializeClassWindow() {
    const px_t
        SECTION_GAP = 6,
        MARGIN = 2,
        WIN_W = 200,
        WIN_H = 200,
        XP_H = 27,
        TREES_Y = XP_H + SECTION_GAP;
    _classWindow = Window::WithRectAndTitle({ 80, 20, WIN_W, WIN_H }, "Class"s);
    const px_t
        TREES_H = _classWindow->contentHeight() - TREES_Y;
    _talentTrees = new Element({ 0, TREES_Y, WIN_W, TREES_H });
    _classWindow->addChild(_talentTrees);

    _classWindow->addChild(new Line(0, XP_H + SECTION_GAP / 2 - 1, WIN_W));

    auto y = MARGIN;
    _levelLabel = new Label({ MARGIN, MARGIN, WIN_W - 2* MARGIN, Element::TEXT_HEIGHT },
            {}, Element::CENTER_JUSTIFIED);
    _classWindow->addChild(_levelLabel);
    y += Element::TEXT_HEIGHT + MARGIN;

    _xpLabel = new Label({ MARGIN, y, WIN_W / 2 - MARGIN, Element::TEXT_HEIGHT }, {}, Element::RIGHT_JUSTIFIED);
    _classWindow->addChild(_xpLabel);

    const auto
        XP_BAR_X = WIN_W / 2 + MARGIN,
        XP_BAR_W = WIN_W / 2 - MARGIN * 2,
        XP_BAR_H = 10;
    _classWindow->addChild(new ProgressBar<XP>({ XP_BAR_X, y, XP_BAR_W, XP_BAR_H }, _xp, _maxXP));
}

void Client::populateClassWindow() {
    _levelLabel->changeText(
        "Level "s + toString(_character.level()) +
        " "s + _character.getClass().name());
    _xpLabel->changeText(toString(_xp) + "/"s + toString(_maxXP) + " experience"s);

    _talentTrees->clearChildren();

    auto &classInfo = _character.getClass();

    const px_t
        GAP = 10,
        TOTAL_GAP_WIDTH = GAP * (classInfo.trees().size()),
        TREE_WIDTH = (_talentTrees->rect().w - TOTAL_GAP_WIDTH) / classInfo.trees().size(),
        TREE_HEIGHT = _talentTrees->rect().h;
    auto x = GAP/2;
    auto treeElems = std::map<std::string, Element *>{};
    auto linesDrawn = size_t{ 0 };
    for (auto &tree : classInfo.trees()) {
        auto treeElem = new Element({x, 0, TREE_WIDTH, 300});
        tree.element = treeElem;
        _talentTrees->addChild(treeElem);

        treeElem->addChild(new Label({ 0, 0, TREE_WIDTH, Element::TEXT_HEIGHT }, tree.name,
                Element::CENTER_JUSTIFIED));

        x += TREE_WIDTH;

        if (linesDrawn < classInfo.trees().size() - 1)
            _talentTrees->addChild(new Line(x + (GAP/2) - 1, 0, TREE_HEIGHT, Element::VERTICAL));
        ++linesDrawn;

        x += GAP;
    }

    for (const auto &tree : classInfo.trees()) {
        auto baseY = 18_px;
        auto yDist = 0_px;
        if (tree.numTiers() > 1)
            yDist = (TREE_HEIGHT - 18 - baseY - GAP/2) / (tree.numTiers() - 1);
        for (auto tierPair : tree.talents) {
            auto tier = tierPair.first;
            auto y = static_cast<px_t>(baseY + tier * yDist);

            const auto &talents = tierPair.second;
            auto x = 0_px;
            auto xDist = 0_px;
            if (talents.size() > 1)
                xDist = (TREE_WIDTH - 18) / (talents.size() - 1);
            else
                x = (TREE_WIDTH - 18) / 2;
            
            for (const auto &talent : talents) {

                void *learnMessageVoidPtr = const_cast<void*>(
                    reinterpret_cast<const void*>(&*talent.learnMessage));
                auto learnSpellButton = new Button({ x, y, 18, 18 }, ""s,
                    this->sendRawMessageStatic, learnMessageVoidPtr);
                learnSpellButton->setTooltip(talent.tooltip());
                if (talent.icon)
                    learnSpellButton->addChild(new Picture(1, 1, talent.icon));

                tree.element->addChild(learnSpellButton);

                x += xDist;
            }


            y += yDist;
        }
    }
}
