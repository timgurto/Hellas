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

    _inventoryWindow = new Window(Rect(LEFT, TOP, WIDTH, HEIGHT), "Inventory");
    inventory->id("inventory");
    _inventoryWindow->addChild(inventory);

    _inventoryWindow->show();
}

void Client::initializeGearWindow(){
    XmlReader xr("client-config.xml");

    GEAR_SLOT_NAMES.push_back("Head");
    GEAR_SLOT_NAMES.push_back("Jewelry");
    GEAR_SLOT_NAMES.push_back("Body");
    GEAR_SLOT_NAMES.push_back("Shoulders");
    GEAR_SLOT_NAMES.push_back("Hands");
    GEAR_SLOT_NAMES.push_back("Feet");
    GEAR_SLOT_NAMES.push_back("Right hand");
    GEAR_SLOT_NAMES.push_back("Left hand");

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
    static const px_t
        STATS_WIDTH = 90,
        STAT_X_GAP = 2;

    _gearWindow = new Window(Rect(100, 100, w + STATS_WIDTH + 2 * STAT_X_GAP, h), "Gear");
    ContainerGrid *gearContainer = new ContainerGrid
            (rows, cols, _character.gear(), GEAR, x, y, gap, false);
    _gearWindow->addChild(new Picture(0, 0, _gearWindowBackground));
    _gearWindow->addChild(gearContainer);

    // Stats display
    Rect labelRect(w + STAT_X_GAP, 0, STATS_WIDTH, Element::TEXT_HEIGHT);
    
    // Username
    _gearWindow->addChild(new LinkedLabel<std::string>
            (labelRect, _username, "", "", Element::CENTER_JUSTIFIED));

    // Class
    labelRect.y += Element::TEXT_HEIGHT;
    _gearWindow->addChild(new LinkedLabel<std::string>
            (labelRect, _character.getClass(), "", "", Element::CENTER_JUSTIFIED));
    

    labelRect.y += Element::TEXT_HEIGHT;
    _gearWindow->addChild(new Label(labelRect, "Health"));
    _gearWindow->addChild(new LinkedLabel<health_t>(labelRect, _character.health(),
                                                    "", "", Element::RIGHT_JUSTIFIED));
    
    labelRect.y += Element::TEXT_HEIGHT;
    _gearWindow->addChild(new Label(labelRect, "Max health"));
    _gearWindow->addChild(new LinkedLabel<health_t>(labelRect, _stats.health,
                                                    "", "", Element::RIGHT_JUSTIFIED));
    
    labelRect.y += Element::TEXT_HEIGHT;
    _gearWindow->addChild(new Label(labelRect, "Damage"));
    _gearWindow->addChild(new LinkedLabel<health_t>(labelRect, _stats.attack,
                                                    "", "", Element::RIGHT_JUSTIFIED));
    
    labelRect.y += Element::TEXT_HEIGHT;
    _gearWindow->addChild(new Label(labelRect, "Attack time"));
    _gearWindow->addChild(new LinkedLabel<ms_t>(labelRect, _stats.attackTime,
                                                    "", "ms", Element::RIGHT_JUSTIFIED));
    
    labelRect.y += Element::TEXT_HEIGHT;
    _gearWindow->addChild(new Label(labelRect, "Speed"));
    _gearWindow->addChild(new LinkedLabel<double>(labelRect, _stats.speed,
                                                    "", "", Element::RIGHT_JUSTIFIED));
}
