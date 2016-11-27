#include "Client.h"
#include "Texture.h"
#include "ui/Picture.h"
#include "ui/Window.h"

void Client::initializeMapWindow(){
    _mapImage = Texture(std::string("Images/map.png"));
    _mapWindow = new Window(Rect((SCREEN_X - _mapImage.width()) / 2,
                                 (SCREEN_Y - _mapImage.height()) / 2,
                                 _mapImage.width(), _mapImage.height()),
                            "Map");
    _mapWindow->addChild(new Picture(0, 0, _mapImage));
    _charPinImage = Texture("Images/mapPinRed.png", Color::MAGENTA);
    _charPin = new Picture(0, 0, _charPinImage);
    _mapWindow->addChild(_charPin);
}

void Client::updateMapWindow(){
    static const double
        MAP_FACTOR_X = 1.0 * _mapX * TILE_W / _mapImage.width(),
        MAP_FACTOR_Y = 1.0 * _mapY * TILE_H / _mapImage.height();
    static px_t
        prevLocX = 0,
        prevLocY = 0;
    px_t
        locX = toInt(_character.location().x / MAP_FACTOR_X),
        locY =toInt( _character.location().y / MAP_FACTOR_Y);

    if (prevLocX != locX || prevLocY != locY){
        _charPin->rect(locX - _charPin->width() / 2,
                       locY - _charPin->height() / 2);
        prevLocX = locX;
        prevLocY = locY;
    }
}
