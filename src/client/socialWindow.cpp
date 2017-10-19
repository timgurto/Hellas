#include "Client.h"
#include "ui/Line.h"

extern Renderer renderer;

static const auto
    GAP = 2_px,
    WIN_WIDTH = 150_px,
    BUTTON_WIDTH = 100_px,
    BUTTON_HEIGHT = 15_px;

void Client::initializeSocialWindow() {
    _socialWindow = Window::WithRectAndTitle( { 400, 100, WIN_WIDTH, 0 }, "Social");

    auto y = GAP;

    const auto
        CITY_SECTION_HEIGHT = 30_px;
    _citySection = new Element{ { 0, y, WIN_WIDTH, CITY_SECTION_HEIGHT } };
    _socialWindow->addChild(_citySection);
    y += _citySection->height() + GAP;

    _socialWindow->addChild(new Line{ 0, y, WIN_WIDTH });
    y += 2 + GAP;

    const px_t
        WARS_HEIGHT = 100;
    _warsList = new List{ {0, y, WIN_WIDTH, WARS_HEIGHT } };
    _socialWindow->addChild(_warsList);
    y += WARS_HEIGHT + GAP;

    _socialWindow->resize(WIN_WIDTH, y);

    refreshCitySection();
    populateWarsList();
}

void Client::refreshCitySection() {
    _citySection->clearChildren();
    auto y = 0_px;

    bool isInCity = !_character.cityName().empty();
    auto cityString = ""s;
    if (isInCity)
        cityString = "City: "s + _character.cityName();
    else
        cityString = "You are not in a city."s;
    _citySection->addChild(new Label{ { GAP, y, WIN_WIDTH, Element::TEXT_HEIGHT }, cityString });
    y += Element::TEXT_HEIGHT + GAP;

    if (isInCity && !_character._isKing) {
        static auto LEAVE_CITY_MESSAGE = compileMessage(CL_LEAVE_CITY);
        _citySection->addChild(new Button{ { GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT }, "Leave city"s,
            sendRawMessageStatic, &LEAVE_CITY_MESSAGE });
    }
}

void Client::populateWarsList() {
    _warsList->clearChildren();
    for (const auto &city : _atWarWithCity)
        _warsList->addChild(new Label{ {}, city });
    for (const auto &player : _atWarWithPlayer)
        _warsList->addChild(new Label{ {}, player });
}
