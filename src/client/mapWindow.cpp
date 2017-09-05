#include "Client.h"
#include "Texture.h"
#include "ui/Picture.h"
#include "ui/Window.h"

void Client::initializeMapWindow(){
    _mapImage = Texture(std::string("Images/map.png"));
    _mapWindow = Window::WithRectAndTitle(Rect((SCREEN_X - _mapImage.width()) / 2,
                                               (SCREEN_Y - _mapImage.height()) / 2,
                                               _mapImage.width(), _mapImage.height()),
                                          "Map");
    _mapWindow->addChild(new Picture(0, 0, _mapImage));

    _mapPinOutlines = new Element(Rect(0, 0, _mapImage.width(), _mapImage.height()));
    _mapPins = new Element(Rect(0, 0, _mapImage.width(), _mapImage.height()));
    _mapWindow->addChild(_mapPinOutlines);
    _mapWindow->addChild(_mapPins);

    _mapWindow->setPreRefreshFunction(updateMapWindow);
}

void Client::updateMapWindow(Element &){
    Client &client = *Client::_instance;

    client._mapPins->clearChildren();
    client._mapPinOutlines->clearChildren();

    for (const auto &objPair : client._objects){
        const auto &object = *objPair.second;
        client.addMapPin(object.location(), object.nameColor());
    }

    for (const auto &pair : client._otherUsers){
        const auto &avatar = *pair.second;
        client.addMapPin(avatar.location(), avatar.nameColor());
    }

    client.addMapPin(client._character.location(), Color::COMBATANT_SELF);
}

void Client::addMapPin(const Point &position, const Color &color){
    static const Rect
        PIN_RECT(0, 0, 1, 1),
        OUTLINE_RECT(-1, -1, 3, 3);

    const double
        MAP_FACTOR_X = 1.0 * _mapX * TILE_W / _mapImage.width(),
        MAP_FACTOR_Y = 1.0 * _mapY * TILE_H / _mapImage.height();

    Point positionInMap(position.x / MAP_FACTOR_X, position.y / MAP_FACTOR_Y);

    _mapPins->addChild(new ColorBlock(PIN_RECT + positionInMap, color));
    _mapPinOutlines->addChild(new ColorBlock(OUTLINE_RECT + positionInMap, Color::OUTLINE));
}
