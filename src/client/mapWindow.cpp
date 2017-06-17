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
}

void Client::updateMapWindow(){
    const double
        MAP_FACTOR_X = 1.0 * _mapX * TILE_W / _mapImage.width(),
        MAP_FACTOR_Y = 1.0 * _mapY * TILE_H / _mapImage.height();
    
    _mapPins->clearChildren();
    _mapPinOutlines->clearChildren();

    static const Rect
        PIN_RECT(0, 0, 1, 1),
        OUTLINE_RECT(-1, -1, 3, 3);

    for (const auto &objPair : _objects){
        const auto &object = *objPair.second;
        const Point mapLoc(object.location().x / MAP_FACTOR_X, object.location().y / MAP_FACTOR_Y);
        _mapPinOutlines->addChild(new ColorBlock(OUTLINE_RECT + mapLoc, Color::OUTLINE));
        _mapPins->addChild(new ColorBlock(PIN_RECT + mapLoc, object.nameColor()));
    }

    const Point mapLoc(_character.location().x / MAP_FACTOR_X, _character.location().y / MAP_FACTOR_Y);
    _mapPins->addChild(new ColorBlock(PIN_RECT + mapLoc, Color::COMBATANT_SELF));
}
