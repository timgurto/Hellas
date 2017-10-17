#include "Client.h"

extern Renderer renderer;

void Client::initializeSocialWindow() {
    const px_t
        GAP = 2,
        BUTTON_WIDTH = 100,
        BUTTON_HEIGHT = Element::TEXT_HEIGHT,
        WIN_WIDTH = 100;

    _socialWindow = Window::WithRectAndTitle( { 400, 100, WIN_WIDTH, 0 }, "Social");

    auto y = GAP;

    // TODO: fix
    if (!_character.cityName().empty() && !_character._isKing) {
        static auto LEAVE_CITY_MESSAGE = compileMessage(CL_LEAVE_CITY);
        _socialWindow->addChild(new Button({ GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT }, "Leave city",
            sendRawMessageStatic, &LEAVE_CITY_MESSAGE));
        y += BUTTON_HEIGHT + GAP;
    }

    const px_t
        WARS_HEIGHT = 100;
    _warsList = new List{ {0, y, WIN_WIDTH, WARS_HEIGHT } };
    _warsList->doNotScrollToTopOnClear();
    _socialWindow->addChild(_warsList);
    y += WARS_HEIGHT + GAP;

    _socialWindow->resize(WIN_WIDTH, y);

    populateWarsList();
}

void Client::populateWarsList() {
    _warsList->clearChildren();
    for (const auto &city : _atWarWithCity)
        _warsList->addChild(new Label{ {}, city });
    for (const auto &player : _atWarWithPlayer)
        _warsList->addChild(new Label{ {}, player });
}
