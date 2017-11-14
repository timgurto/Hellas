#include "Client.h"
#include "../XmlReader.h"
#include "../server/User.h"
#include "ui/ContainerGrid.h"
#include "ui/Label.h"
#include "ui/Line.h"
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
    Element *gearWindow, Color &lineColor = Element::FONT_COLOR) {
    const auto
        X = 2_px,
        W = 98_px;
    auto labelRect = Rect{ X, y, W, Element::TEXT_HEIGHT };
    auto nameLabel = new Label(labelRect, label);
    nameLabel->setColor(lineColor);
    gearWindow->addChild(nameLabel);
    auto valueLabel = new LinkedLabel<T>(labelRect, value, prefix, suffix, Element::RIGHT_JUSTIFIED);
    valueLabel->setColor(lineColor);
    gearWindow->addChild(valueLabel);

    y += Element::TEXT_HEIGHT;
}

static void addGap(px_t &y, Element *gearWindow){
    const auto
        X = 2_px,
        W = 98_px;
    gearWindow->addChild(new Line(X-2, y, W+4));
    y += 2;
}

void Client::initializeGearWindow(){
    _gearWindowBackground = Texture(std::string("Images/gearWindow.png"), Color::MAGENTA);

    const auto
        STATS_WIDTH = 100, STAT_X_GAP = 2_px,
        gridX = STATS_WIDTH + STAT_X_GAP, gridY = -1_px,
        w = 16_px, h = 80_px,
        gap = 0_px;
    const auto
        rows = 8, cols = 1;

    _gearWindow->rect(0, 37, w + STATS_WIDTH + 2 * STAT_X_GAP, h);
    _gearWindow->setTitle("Gear and Stats");
    ContainerGrid *gearContainer = new ContainerGrid
            (rows, cols, _character.gear(), GEAR, gridX, gridY, gap, false);
    _gearWindow->addChild(new Picture(gridX, 0, _gearWindowBackground));
    _gearWindow->addChild(gearContainer);

    // Stats display
    Rect labelRect(STAT_X_GAP, 0, STATS_WIDTH, Element::TEXT_HEIGHT);

    // Stats
    auto y = labelRect.y;
    addStat("Max health",       _stats.maxHealth,       {},     {},     y, _gearWindow, Color::COMBATANT_SELF);
    addStat("Health regen",     _stats.hps,             {},     "/s",   y, _gearWindow, Color::COMBATANT_SELF);
    addStat("Max energy",       _stats.maxEnergy,       {},     {},     y, _gearWindow, Color::ENERGY);
    addStat("Energy regen",     _stats.eps,             {},     "/s",   y, _gearWindow, Color::ENERGY);
    addGap(y, _gearWindow);
    addStat("Hit chance",       _stats.hit,             {},     "%",    y, _gearWindow);
    addStat("Crit chance",      _stats.crit,            {},     "%",    y, _gearWindow);
    addStat("Physical damage",  _stats.physicalDamage,  "+",    {},     y, _gearWindow);
    addStat("Magic damage",     _stats.magicDamage,     "+",    {},     y, _gearWindow);
    addStat("Healing power",    _stats.healing,         "+",    {},     y, _gearWindow);
    addGap(y, _gearWindow);
    addStat("Armor",            _stats.armor,           {},     "%",    y, _gearWindow);
    addStat("Air resistance",   _stats.airResist,       {},     "%",    y, _gearWindow, Color::AIR);
    addStat("Earth resistance", _stats.earthResist,     {},     "%",    y, _gearWindow, Color::EARTH);
    addStat("Fire resistance",  _stats.fireResist,      {},     "%",    y, _gearWindow, Color::FIRE);
    addStat("Water resistance", _stats.waterResist,     {},     "%",    y, _gearWindow, Color::WATER);
    addStat("Crit avoidance",   _stats.critResist,      {},     "%",    y, _gearWindow);
    addStat("Dodge chance",     _stats.dodge,           {},     "%",    y, _gearWindow);
    addStat("Block chance",     _stats.block,           {},     "%",    y, _gearWindow);
    addStat("Block value",      _stats.blockValue,      {},     {},     y, _gearWindow);
    addGap(y, _gearWindow);
    addStat("Weapon damage",    _stats.attack,          {},     {},     y, _gearWindow);
    addStat("Speed",            _stats.speed,           {},     {},     y, _gearWindow);

    y += 2;
    _gearWindow->height(y);

    _gearWindow->addChild(new Line(STATS_WIDTH, 0, y, Element::VERTICAL));
}
