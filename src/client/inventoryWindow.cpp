#include "Client.h"
#include "../XmlReader.h"
#include "../server/User.h"
#include "ui/Container.h"
#include "ui/Window.h"

void Client::initializeInventoryWindow(){
    static const px_t
        COLS = 4,
        ROWS = (Client::INVENTORY_SIZE - 1) / COLS + 1;
    Container *inventory = new Container(ROWS, COLS, _inventory, INVENTORY);
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

void Client::initializeGearWindow(){
    XmlReader xr("client-config.xml");
    px_t
        x = 0, y = 0,
        w = 0, h = 0,
        gap = 0, rows = 0, cols = 0;
    auto elem = xr.findChild("gearWindow");
    if (elem != nullptr){
        xr.findAttr(elem, "width", w);
        xr.findAttr(elem, "height", h);
        xr.findAttr(elem, "gridX", x);
        xr.findAttr(elem, "gridY", y);
        xr.findAttr(elem, "gap", gap);
        xr.findAttr(elem, "rows", rows);
        xr.findAttr(elem, "cols", cols);
    }

    _gearWindow = new Window(Rect(100, 100, w, h), "Gear");
    Container *gearContainer = new Container(rows, cols, _gear, GEAR, x, y, gap, false);
    _gearWindow->addChild(new Picture(0, 0, _gearWindowBackground));
    _gearWindow->addChild(gearContainer);
}
