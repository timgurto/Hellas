#include "Client.h"
#include "../XmlReader.h"
#include "../server/User.h"
#include "ui/ContainerGrid.h"
#include "ui/Label.h"
#include "ui/LinkedLabel.h"
#include "ui/Window.h"

void Client::initializeInventoryWindow(){
    static const px_t
        COLS = 5,
        ROWS = (Client::INVENTORY_SIZE - 1) / COLS + 1;
    ContainerGrid *inventory = new ContainerGrid(ROWS, COLS, _inventory, INVENTORY);
    const px_t
        HEIGHT = inventory->height(),
        WIDTH = inventory->width(),
        LEFT = SCREEN_X - WIDTH - 1,
        TOP = SCREEN_Y - HEIGHT - 16;

    _inventoryWindow = Window::WithRectAndTitle(Rect(LEFT, TOP, WIDTH, HEIGHT), "Inventory");
    inventory->id("inventory");
    _inventoryWindow->addChild(inventory);
}

void Client::initializeGearWindow(Client &client){
    client.initializeGearWindow();
}

void Client::initializeGearWindow(){
    _gearWindowBackground = Texture(std::string("Images/gearWindow.png"), Color::MAGENTA);

    px_t
        x = 0, y = 0,
        w = 0, h = 0,
        gap = 0, rows = 0, cols = 0;
    XmlReader xr("client-config.xml");
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
    static const px_t
        STATS_WIDTH = 90,
        STAT_X_GAP = 2;

    _gearWindow->rect(100, 100, w + STATS_WIDTH + 2 * STAT_X_GAP, h);
    _gearWindow->setTitle("Gear");
    ContainerGrid *gearContainer = new ContainerGrid
            (rows, cols, _character.gear(), GEAR, x, y, gap, false);
    _gearWindow->addChild(new Picture(0, 0, _gearWindowBackground));
    _gearWindow->addChild(gearContainer);

    // Stats display
    Rect labelRect(w + STAT_X_GAP, 0, STATS_WIDTH, Element::TEXT_HEIGHT);
    
    // Username
    _gearWindow->addChild(new LinkedLabel<std::string>
            (labelRect, _username, "", "", Element::CENTER_JUSTIFIED));
    labelRect.y += Element::TEXT_HEIGHT;

    // Class
    _gearWindow->addChild(new LinkedLabel<std::string>
            (labelRect, _character.getClass(), "", "", Element::CENTER_JUSTIFIED));    
    labelRect.y += Element::TEXT_HEIGHT;

    // Stats
    _gearWindow->addChild(new Label(labelRect, "Max health"));
    _gearWindow->addChild(new LinkedLabel<Hitpoints>(labelRect, _stats.health,
                                                    "", "", Element::RIGHT_JUSTIFIED));
    labelRect.y += Element::TEXT_HEIGHT;

    _gearWindow->addChild(new Label(labelRect, "Max energy"));
    _gearWindow->addChild(new LinkedLabel<Hitpoints>(labelRect, _stats.energy,
                                                                "", "", Element::RIGHT_JUSTIFIED));
    labelRect.y += Element::TEXT_HEIGHT;

    _gearWindow->addChild(new Label(labelRect, "Damage"));
    _gearWindow->addChild(new LinkedLabel<Hitpoints>(labelRect, _stats.attack,
                                                    "", "", Element::RIGHT_JUSTIFIED));
    labelRect.y += Element::TEXT_HEIGHT;

    _gearWindow->addChild(new Label(labelRect, "Speed"));
    _gearWindow->addChild(new LinkedLabel<double>(labelRect, _stats.speed,
                                                    "", "", Element::RIGHT_JUSTIFIED));
    labelRect.y += Element::TEXT_HEIGHT;


    _gearWindow->height(labelRect.y);
}
