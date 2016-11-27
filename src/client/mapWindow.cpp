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
    updateMapWindow();
}

void Client::updateMapWindow(){
    static px_t
        prevLocX = toInt(_character.location().x),
        prevLocY = toInt(_character.location().y);
    static Point prevLoc = _character.location();
    px_t
        locX = _character.location().x,
        locY = _character.location().y;

    if (prevLocX != locX || prevLocY != locY){

        prevLocX = locX;
        prevLocY = locY;
    }
}
