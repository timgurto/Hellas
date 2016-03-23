// (C) 2015 Tim Gurto

#include "Client.h"
#include "../server/User.h"
#include "ui/Container.h"
#include "ui/Window.h"

void Client::initializeInventoryWindow(){
    static const px_t
        COLS = 4,
        ROWS = (Client::INVENTORY_SIZE - 1) / COLS + 1;
    Container *inventory = new Container(ROWS, COLS, _inventory);
    const px_t
        HEIGHT = inventory->height(),
        WIDTH = inventory->width(),
        LEFT = 640 - WIDTH - 1,
        TOP = 360 - HEIGHT - 16;

    _inventoryWindow = new Window(Rect(LEFT, TOP, WIDTH, HEIGHT), "Inventory");
    inventory->id("inventory");
    _inventoryWindow->addChild(inventory);

    _inventoryWindow->show();
}
