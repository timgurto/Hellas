#include "Client.h"

void Client::initializeClassWindow() {
    _classWindow = Window::WithRectAndTitle({ 80, 20, 450, 300 }, "Class"s);
}

void Client::populateClassWindow() {
    _classWindow->clearChildren();

    const ClassInfo &classInfo = _character.getClass();

    const auto TREE_WIDTH = 150_px;
    auto x = 0_px;
    auto treeElems = std::map<std::string, Element *>{};
    auto treeY = std::map<std::string, px_t>{};
    for (const auto &tree : classInfo.trees()) {
        auto treeElem = new Element({x, 0, TREE_WIDTH, 300});
        treeElems[tree] = treeElem;
        treeY[tree] = 18_px;
        _classWindow->addChild(treeElem);

        treeElem->addChild(new Label({ 0, 0, TREE_WIDTH, Element::TEXT_HEIGHT }, tree, Element::CENTER_JUSTIFIED));

        x += TREE_WIDTH;
    }

    for (const auto spell : classInfo.spells()) {
        auto treeIt = treeElems.find(spell->tree());
        if (treeIt == treeElems.end())
            continue;
        auto &y = treeY[spell->tree()];

        void *learnMessageVoidPtr = const_cast<void*>(
            reinterpret_cast<const void*>(&spell->learnMessage()));
        auto learnSpellButton = new Button({ 0, y, 18, 18 }, ""s,
            this->sendRawMessageStatic, learnMessageVoidPtr);
        learnSpellButton->setTooltip(spell->name());
        learnSpellButton->addChild(new Picture(1, 1, spell->icon()));

        treeIt->second->addChild(learnSpellButton);
        y += 18;
    }
}
