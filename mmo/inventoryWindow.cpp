// (C) 2015 Tim Gurto

#include "Client.h"
#include "Container.h"
#include "Window.h"

void Client::initializeInventoryWindow(){
    static const int
        ROWS = 2,
        COLS = 3;
    Container *inventory = new Container(ROWS, COLS, _inventory);
    const int
        MARGIN = 1,
        HEIGHT = inventory->height() * 2 + MARGIN * 2 + Window::HEADING_HEIGHT,
        WIDTH = inventory->width() * 2 + MARGIN * 2,
        LEFT = 50,
        TOP = 300;

    _inventoryWindow = new Window(makeRect(TOP, LEFT, WIDTH, HEIGHT), "Inventory");
    _inventoryWindow->addChild(inventory);

    _inventoryWindow->show();
}
