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
        LEFT = 50,
        TOP = 300;
        HEIGHT = inventory->height(),
        WIDTH = inventory->width(),

    _inventoryWindow = new Window(makeRect(LEFT, TOP, WIDTH, HEIGHT), "Inventory");
    _inventoryWindow->addChild(inventory);

    _inventoryWindow->show();
}
