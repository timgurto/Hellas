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

template<typename T>
static void addStat(const std::string &label, const T &value,
    const std::string &prefix, const std::string &suffix, px_t &y,
    Element *gearWindow) {
    const auto
        X = 38_px,
        W = 100_px;
    auto labelRect = Rect{ X, y, W, Element::TEXT_HEIGHT };
    gearWindow->addChild(new Label(labelRect, label));
    gearWindow->addChild(new LinkedLabel<T>(labelRect, value, prefix, suffix,
        Element::RIGHT_JUSTIFIED));

    y += Element::TEXT_HEIGHT;
}

void Client::initializeGearWindow(){
    _gearWindowBackground = Texture(std::string("Images/gearWindow.png"), Color::MAGENTA);

    px_t
        gridX = 0, gridY = 0,
        w = 0, h = 0,
        gap = 0, rows = 0, cols = 0;
    XmlReader xr("client-config.xml");
    auto elem = xr.findChild("gearWindow");
    if (elem != nullptr){
        xr.findAttr(elem, "width", w);
        xr.findAttr(elem, "height", h);
        xr.findAttr(elem, "gridX", gridX);
        xr.findAttr(elem, "gridY", gridY);
        xr.findAttr(elem, "gap", gap);
        xr.findAttr(elem, "rows", rows);
        xr.findAttr(elem, "cols", cols);
    }
    static const px_t
        STATS_WIDTH = 100,
        STAT_X_GAP = 2;

    _gearWindow->rect(100, 100, w + STATS_WIDTH + 2 * STAT_X_GAP, h);
    _gearWindow->setTitle("Gear");
    ContainerGrid *gearContainer = new ContainerGrid
            (rows, cols, _character.gear(), GEAR, gridX, gridY, gap, false);
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
    auto y = labelRect.y;
    addStat("Max health",       _stats.health,          {},     {},     y, _gearWindow);
    addStat("Max energy",       _stats.energy,          {},     {},     y, _gearWindow);
    addStat("Health regen",     _stats.hps,             {},     "/s",   y, _gearWindow);
    addStat("Energy regen",     _stats.eps,             {},     "/s",   y, _gearWindow);
    addStat("Hit chance",       _stats.hit,             {},     "%",    y, _gearWindow);
    addStat("Crit chance",      _stats.crit,            {},     "%",    y, _gearWindow);
    addStat("Crit avoidance",   _stats.critResist,      {},     "%",    y, _gearWindow);
    addStat("Dodge chance",     _stats.dodge,           {},     "%",    y, _gearWindow);
    addStat("Block chance",     _stats.block,           {},     "%",    y, _gearWindow);
    addStat("Block value",      _stats.blockValue,      {},     {},     y, _gearWindow);
    addStat("Air resistance",   _stats.airResist,       {},     "%",    y, _gearWindow);
    addStat("Earth resistance", _stats.earthResist,     {},     "%",    y, _gearWindow);
    addStat("Fire resistance",  _stats.fireResist,      {},     "%",    y, _gearWindow);
    addStat("Water resistance", _stats.waterResist,     {},     "%",    y, _gearWindow);
    addStat("Physical damage",  _stats.physicalDamage,  "+",    {},     y, _gearWindow);
    addStat("Magic damage",     _stats.magicDamage,     "+",    {},     y, _gearWindow);
    addStat("Healing power",    _stats.healing,         "+",    {},     y, _gearWindow);
    addStat("Weapon damage",    _stats.attack,          {},     {},     y, _gearWindow);
    addStat("Speed",            _stats.speed,           {},     {},     y, _gearWindow);

    y += 2;
    _gearWindow->height(y);
}
