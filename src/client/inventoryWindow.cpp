#include "../XmlReader.h"
#include "../server/User.h"
#include "Client.h"
#include "ui/ContainerGrid.h"
#include "ui/Label.h"
#include "ui/Line.h"
#include "ui/LinkedLabel.h"
#include "ui/Window.h"

void Client::initializeInventoryWindow() {
  static const px_t COLS = 5, ROWS = (Client::INVENTORY_SIZE - 1) / COLS + 1;
  ContainerGrid *inventory =
      new ContainerGrid(*this, ROWS, COLS, _inventory, Serial::Inventory());
  const px_t HEIGHT = inventory->height() + 1, WIDTH = inventory->width(),
             LEFT = SCREEN_X - WIDTH - 1, TOP = SCREEN_Y - HEIGHT - 28;

  _inventoryWindow = Window::WithRectAndTitle({LEFT, TOP, WIDTH, HEIGHT},
                                              "Inventory", mouse());
  inventory->id("inventory");
  _inventoryWindow->addChild(inventory);
}

void Client::initializeGearWindow(Client &client) {
  client.initializeGearWindow();
}

template <typename T>
static void addStat(const std::string &label, const T &value,
                    const std::string &prefix, const std::string &suffix,
                    px_t &y, Element *gearWindow,
                    const std::string &tooltip = {},
                    const Color &lineColor = Element::FONT_COLOR) {
  auto shouldAddTooltip = !tooltip.empty();

  const auto X = 2_px, W = 98_px;
  auto labelRect = ScreenRect{X, y, W, Element::TEXT_HEIGHT};
  auto nameLabel = new Label(labelRect, label);
  nameLabel->setColor(lineColor);
  if (shouldAddTooltip) nameLabel->setTooltip(tooltip);
  gearWindow->addChild(nameLabel);
  auto valueLabel = new LinkedLabel<T>(labelRect, value, prefix, suffix,
                                       Element::RIGHT_JUSTIFIED);
  valueLabel->setColor(lineColor);
  if (shouldAddTooltip) valueLabel->setTooltip(tooltip);
  gearWindow->addChild(valueLabel);

  y += Element::TEXT_HEIGHT;
}

static void addGap(px_t &y, Element *gearWindow) {
  const auto X = 2_px, W = 98_px;
  gearWindow->addChild(new Line(X - 2, y, W + 4));
  y += 2;
}

void Client::initializeGearWindow() {
  _gearWindowBackground =
      Texture(std::string("Images/gearWindow.png"), Color::MAGENTA);

  const auto STATS_WIDTH = 100, STAT_X_GAP = 2_px,
             gridX = STATS_WIDTH + STAT_X_GAP, gridY = -1_px, w = 16_px,
             h = 80_px, gap = 0_px;
  const auto rows = 8, cols = 1;

  _gearWindow->rect(0, 37, w + STATS_WIDTH + 2 * STAT_X_GAP, h);
  _gearWindow->setTitle("Gear and Stats");
  ContainerGrid *gearContainer =
      new ContainerGrid(*this, rows, cols, _character.gear(), Serial::Gear(),
                        gridX, gridY, gap, false);
  _gearWindow->addChild(new Picture(gridX, 0, _gearWindowBackground));
  _gearWindow->addChild(gearContainer);

  // Stats display
  auto labelRect = ScreenRect{STAT_X_GAP, 0, STATS_WIDTH, Element::TEXT_HEIGHT};

  // Stats
  auto y = labelRect.y;
  addStat("Max health", _stats.maxHealth, {}, {}, y, _gearWindow, {},
          Color::STAT_HEALTH);
  addStat("Health regen", _stats.hps, {}, "/s", y, _gearWindow, {},
          Color::STAT_HEALTH);
  addStat("Max energy", _stats.maxEnergy, {}, {}, y, _gearWindow, {},
          Color::STAT_ENERGY);
  addStat("Energy regen", _stats.eps, {}, "/s", y, _gearWindow, {},
          Color::STAT_ENERGY);
  addGap(y, _gearWindow);
  addStat("Hit chance", _stats.hit, {}, "%", y, _gearWindow,
          "Extra chance to hit.  You have a baseline 10% chance to miss "
          "enemies at the same level as you.");
  addStat("Crit chance", _stats.crit, {}, "%", y, _gearWindow,
          "Chance that an attack will do double damage.");
  addStat("Physical damage", _stats.physicalDamage, "+", {}, y, _gearWindow,
          "Additional damage added to physical attacks and spells.");
  addStat("Magic damage", _stats.magicDamage, "+", {}, y, _gearWindow,
          "Additional damage added to magical attacks and spells");
  addStat("Healing power", _stats.healing, "+", {}, y, _gearWindow,
          "Additional amount added to healing spells.");
  addGap(y, _gearWindow);
  addStat("Armor", _stats.armor, {}, "%", y, _gearWindow,
          "A flat reduction applied to all incoming physical damage.");
  addStat("Air resistance", _stats.airResist, {}, "%", y, _gearWindow,
          "A flat reduction applied to all incoming air damage.",
          Color::STAT_AIR);
  addStat("Earth resistance", _stats.earthResist, {}, "%", y, _gearWindow,
          "A flat reduction applied to all incoming earth damage.",
          Color::STAT_EARTH);
  addStat("Fire resistance", _stats.fireResist, {}, "%", y, _gearWindow,
          "A flat reduction applied to all incoming fire damage.",
          Color::STAT_FIRE);
  addStat("Water resistance", _stats.waterResist, {}, "%", y, _gearWindow,
          "A flat reduction applied to all incoming water damage.",
          Color::STAT_WATER);
  addStat("Crit avoidance", _stats.critResist, {}, "%", y, _gearWindow,
          "Lowers the chance for incoming attacks to be critical hits.");
  addStat("Dodge chance", _stats.dodge, {}, "%", y, _gearWindow,
          "Chance to outright avoid hand-to-hand physical attacks.");
  addStat("Block chance", _stats.block, {}, "%", y, _gearWindow,
          "Chance to reduce the damage of physical attacks by a flat amount.");
  addStat("Block value", _stats.blockValue, {}, {}, y, _gearWindow,
          "How much damage is reduced when an attack is blocked.");
  addGap(y, _gearWindow);
  addStat("Follower limit", _stats.followerLimit, {}, {}, y, _gearWindow,
          "The maximum number of pets that can follow you.");
  addStat("Run speed", _displaySpeed, {}, " podes/s", y, _gearWindow,
          "How fast you run when not in a vehicle.");

  y += 2;
  _gearWindow->height(y);

  _gearWindow->addChild(new Line(STATS_WIDTH, 0, y, Element::VERTICAL));
}
