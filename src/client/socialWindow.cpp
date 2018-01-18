#include "Client.h"
#include "ui/Line.h"

extern Renderer renderer;

static const auto
    GAP = 2_px,
    WIN_WIDTH = 200_px,
    BUTTON_WIDTH = 100_px,
    BUTTON_HEIGHT = 15_px,
    WAR_ROW_HEIGHT = 15_px;

static Texture cityIcon, playerIcon;

void Client::initializeSocialWindow() {
    _socialWindow = Window::WithRectAndTitle( { 400, 100, WIN_WIDTH, 0 }, "Social"s);
    cityIcon = { "Images/ui/city.png"s };
    playerIcon = { "Images/ui/player.png"s };

    auto y = GAP;

    const auto
        CITY_SECTION_HEIGHT = 30_px;
    _citySection = new Element{ { 0, y, WIN_WIDTH, CITY_SECTION_HEIGHT } };
    _socialWindow->addChild(_citySection);
    y += _citySection->height() + GAP;

    _socialWindow->addChild(new Line{ 0, y, WIN_WIDTH });
    y += 2 + GAP;

    _socialWindow->addChild(new Label{ {GAP, y, WIN_WIDTH, WAR_ROW_HEIGHT}, "Wars:"s });
    y += WAR_ROW_HEIGHT;

    const px_t
        WARS_HEIGHT = 100;
    _warsList = new List{ {0, y, WIN_WIDTH, WARS_HEIGHT }, WAR_ROW_HEIGHT };
    _socialWindow->addChild(_warsList);
    y += WARS_HEIGHT + GAP;

    _socialWindow->resize(WIN_WIDTH, y);

    refreshCitySection();
    populateWarsList();
}

void Client::cleanupSocialWindow() {
    cityIcon = {};
    playerIcon = {};
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
        // Static, so that the button still has something to point to after this function exits
        static auto LEAVE_CITY_MESSAGE = compileMessage(CL_LEAVE_CITY);
        _citySection->addChild(new Button{ { GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT }, "Leave city"s,
            sendRawMessageStatic, &LEAVE_CITY_MESSAGE });
    }
}

enum BelligerentType {
    CITY,
    PLAYER
};

Element *createWarRow(const std::string &name, BelligerentType belligerentType,
    BelligerentType yourBelligerentType, PeaceState state, bool isActive = true) {
    const auto
        ICON_W = 12,
        NAME_W = 80_px,
        BUTTON_W = 90_px;
    auto row = new Element;
    auto x = px_t{ GAP };

    const auto &icon = belligerentType == CITY ? cityIcon : playerIcon;
    row->addChild(new Picture{ {x, 1, icon.width(), icon.height()}, icon });
    x += ICON_W;

    auto label = new Label{ { x, 0, NAME_W, WAR_ROW_HEIGHT } , name };
    if (!isActive) {
        label->setColor(Color::ELEMENT_SHADOW_LIGHT);
        label->setTooltip("This personal war is inactive while you are in a city.");
    }
    row->addChild(label);
    x += NAME_W;

    // Peace button
    if (yourBelligerentType == PLAYER || Client::instance().character().isKing()) {
        switch (state) {
        case NO_PEACE_PROPOSED:
        {
            auto buttonFunc = Button::clickFun_t{};
            if (yourBelligerentType == PLAYER) {
                if (belligerentType == PLAYER)
                    buttonFunc = Client::sendMessageWithString<CL_SUE_FOR_PEACE_WITH_PLAYER>;
                else
                    buttonFunc = Client::sendMessageWithString<CL_SUE_FOR_PEACE_WITH_CITY>;
            } else {
                if (belligerentType == PLAYER)
                    buttonFunc = Client::sendMessageWithString<CL_SUE_FOR_PEACE_WITH_PLAYER_AS_CITY>;
                else
                    buttonFunc = Client::sendMessageWithString<CL_SUE_FOR_PEACE_WITH_CITY_AS_CITY>;
            }
            row->addChild(new Button({ x, 0, BUTTON_W, WAR_ROW_HEIGHT }, "Sue for peace"s,
                buttonFunc, &const_cast<std::string &>(name)));
            break;
        }
        case PEACE_PROPOSED_BY_YOU:
            row->addChild(new Button({ x, 0, BUTTON_W, WAR_ROW_HEIGHT }, "Revoke peace offer"s,
                Client::sendMessageWithString<CL_CANCEL_PEACE_OFFER_TO_PLAYER>,
                &const_cast<std::string &>(name)));
            break;
        case PEACE_PROPOSED_BY_HIM:
            row->addChild(new Button({ x, 0, BUTTON_W, WAR_ROW_HEIGHT }, "Accept peace offer"s,
                Client::sendMessageWithString<CL_ACCEPT_PEACE_OFFER_WITH_PLAYER>,
                &const_cast<std::string &>(name)));
            break;
        }
    }

    return row;
}

void Client::populateWarsList() {
    _warsList->clearChildren();

    auto isInCity = !this->_character.cityName().empty();
    if (isInCity) {
        for (const auto &pair : _cityWarsAgainstCities)
            _warsList->addChild(createWarRow(pair.first, CITY, CITY, pair.second));
        for (const auto &pair : _cityWarsAgainstPlayers)
            _warsList->addChild(createWarRow(pair.first, PLAYER, CITY, pair.second));
    }

    for (const auto &pair : _warsAgainstCities)
        _warsList->addChild(createWarRow(pair.first, CITY, PLAYER, pair.second, !isInCity));
    for (const auto &pair : _warsAgainstPlayers)
        _warsList->addChild(createWarRow(pair.first, PLAYER, PLAYER, pair.second, !isInCity));
}
