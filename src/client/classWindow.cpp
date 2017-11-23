#include "Client.h"
#include "ui/Line.h"

void Client::initializeClassWindow() {
    _classWindow = Window::WithRectAndTitle({ 80, 20, 210, 200 }, "Class"s);
}

void Client::populateClassWindow() {
    _classWindow->clearChildren();

    auto &classInfo = _character.getClass();

    const px_t
        GAP = 10,
        TOTAL_GAP_WIDTH = GAP * (classInfo.trees().size()),
        TREE_WIDTH = (_classWindow->contentWidth() - TOTAL_GAP_WIDTH) / classInfo.trees().size(),
        TREE_HEIGHT = _classWindow->contentHeight();
    auto x = GAP/2;
    auto treeElems = std::map<std::string, Element *>{};
    auto linesDrawn = size_t{ 0 };
    for (auto &tree : classInfo.trees()) {
        auto treeElem = new Element({x, 0, TREE_WIDTH, 300});
        tree.element = treeElem;
        _classWindow->addChild(treeElem);

        treeElem->addChild(new Label({ 0, 0, TREE_WIDTH, Element::TEXT_HEIGHT }, tree.name,
                Element::CENTER_JUSTIFIED));

        x += TREE_WIDTH;

        if (linesDrawn < classInfo.trees().size() - 1)
            _classWindow->addChild(new Line(x + (GAP/2) - 1, 0, TREE_HEIGHT, Element::VERTICAL));
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
                learnSpellButton->setTooltip(talent.name);
                if (talent.icon)
                    learnSpellButton->addChild(new Picture(1, 1, *talent.icon));

                tree.element->addChild(learnSpellButton);

                x += xDist;
            }


            y += yDist;
        }
    }
}
