#include "Client.h"

void Client::initializeClassWindow() {
    _classWindow = Window::WithRectAndTitle({ 80, 20, 500, 300 }, "Class"s);
}

void Client::populateClassWindow() {
    _classWindow->clearChildren();

    auto y = 0_px;
    for (const auto spell : _character.getClass().spells()) {
        void *learnMessageVoidPtr = const_cast<void*>(
            reinterpret_cast<const void*>(&spell->learnMessage()));
        auto learnSpellButton = new Button({ 0, y, 18, 18 }, ""s,
            this->sendRawMessageStatic, learnMessageVoidPtr);
        learnSpellButton->setTooltip(spell->name());
        learnSpellButton->addChild(new Picture(1, 1, spell->icon()));

        _classWindow->addChild(learnSpellButton);
        y += 18;
    }
}
