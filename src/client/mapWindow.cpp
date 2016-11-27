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
}
