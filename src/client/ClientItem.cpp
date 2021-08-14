#include "ClientItem.h"

#include "../ItemClass.h"
#include "../XmlReader.h"
#include "Client.h"
#include "Tooltip.h"
#include "Unlocks.h"

std::map<int, size_t> ClientItem::gearDrawOrder;
std::vector<ScreenPoint> ClientItem::gearOffsets(Client::GEAR_SLOTS);

ClientItem::ClientItem() : Item({}) {}

ClientItem::ClientItem(const Client &client, const std::string &id,
                       const std::string &name)
    : _client(&client), Item(id), _name(name), _constructsObject(nullptr) {}

void ClientItem::icon(const std::string &filename) {
  static const std::string prefix = "Images/Items/";
  _icon = {prefix + filename};

  if (!_icon) _icon = {prefix + "none"};
}

void ClientItem::gearImage(const std::string &filename) {
  static const std::string prefix = "Images/Gear/", suffix = ".png";
  _gearImage = Texture(prefix + filename + suffix, Color::MAGENTA);
}

const ClientItem *toClientItem(const Item *item) {
  return dynamic_cast<const ClientItem *>(item);
}

static ScreenPoint toScreenPoint(const MapPoint &rhs) {
  return {toInt(rhs.x), toInt(rhs.y)};
}

void ClientItem::draw(const ScreenPoint &screenLoc) const {
  if (_gearSlot != Item::NOT_GEAR && _gearImage) {
    ScreenPoint drawLoc = _drawLoc +                // The item's offset
                          gearOffsets[_gearSlot] +  // The slot's offset
                          screenLoc;  // The avatar's location on the screen
    _gearImage.draw(drawLoc);
  }
}

void ClientItem::init() {
  auto xr = XmlReader::FromFile("client-config.xml");
  auto elem = xr.findChild("gearDisplay");
  if (elem == nullptr) return;
  for (auto slot : xr.getChildren("slot", elem)) {
    size_t slotNum;
    if (!xr.findAttr(slot, "num", slotNum)) continue;

    // Offsets
    xr.findAttr(slot, "midX", gearOffsets[slotNum].x);
    xr.findAttr(slot, "midY", gearOffsets[slotNum].y);

    // Draw order.  Without this, gear for this slot won't be drawn.
    size_t order;
    if (xr.findAttr(slot, "drawOrder", order)) gearDrawOrder[order] = slotNum;
  }
}

const Tooltip &ClientItem::tooltip() const {
  if (_tooltip.hasValue()) return _tooltip.value();

  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();

  tooltip.setColor(nameColor());
  tooltip.addLine(_name);

  // Gear slot/stats
  if (_gearSlot != Client::GEAR_SLOTS) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_BODY);
    tooltip.addLine("Gear: "s + Client::GEAR_SLOT_NAMES[_gearSlot]);

    if (hasLvlReq()) {
      if (_client->character().level() < lvlReq())
        tooltip.setColor(Color::TOOLTIP_BAD);
      tooltip.addLine("Requires level "s + toString(lvlReq()));
    }

    tooltip.setColor(Color::TOOLTIP_BODY);
    tooltip.addLines(_stats.toStrings());

    if (weaponRange() > Podes::MELEE_RANGE.toPixels())
      tooltip.addLine("Range: "s + toString(Podes::FromPixels(weaponRange())) +
                      " podes");

    if (usesAmmo()) {
      auto &ammoType = dynamic_cast<const ClientItem &>(*weaponAmmo());
      tooltip.addLine("Each attack consumes a "s + ammoType.name());
    }
  }

  // Tags
  tooltip.addTags(*this, _client->gameData.tagNames);

  // Construction
  if (_constructsObject != nullptr) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
    tooltip.addLine(std::string("Right-click to place ") +
                    _constructsObject->name() + ":");
    tooltip.embed(_constructsObject->constructionTooltip(*_client));
  }

  // Spell
  if (castsSpellOnUse()) {
    if (spellToCastOnUse() == "teachRecipe") {
      auto recipeID = this->spellArg();
      const auto it = _client->gameData.recipes.find(recipeID);
      if (it != _client->gameData.recipes.end()) {
        const auto &recipe = *it;
        tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
        tooltip.addLine("Right-click: learn to craft this item.");

        tooltip.addRecipe(recipe, _client->gameData.tagNames);
      }
    } else {
      auto it = _client->gameData.spells.find(spellToCastOnUse());
      if (it == _client->gameData.spells.end()) {
        _client->showErrorMessage("Can't find spell: "s + spellToCastOnUse(),
                                  Color::CHAT_ERROR);
      } else {
        tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
        tooltip.addLine("Right-click: "s +
                        it->second->createEffectDescription());
      }
    }
  }

  // Unlocks
  auto acquireInfo =
      _client->gameData.unlocks.getEffectInfo({Unlocks::ACQUIRE, id()});
  if (acquireInfo.hasEffect) {
    tooltip.addGap();
    tooltip.setColor(acquireInfo.color);
    tooltip.addLine(acquireInfo.message);
  }

  return tooltip;
}

bool ClientItem::canUse() const {
  return _constructsObject != nullptr || castsSpellOnUse();
}

bool ClientItem::shouldWarnBeforeScrapping() const {
  switch (_quality) {
    case COMMON:
      return false;
    case UNCOMMON:
    case RARE:
    case EPIC:
    case LEGENDARY:
    default:
      return true;
  }
}

Color ClientItem::nameColor() const {
  switch (_quality) {
    case COMMON:
      return Color::ITEM_QUALITY_COMMON;
    case UNCOMMON:
      return Color::ITEM_QUALITY_UNCOMMON;
    case RARE:
      return Color::ITEM_QUALITY_RARE;
    case EPIC:
      return Color::ITEM_QUALITY_EPIC;
    case LEGENDARY:
      return Color::ITEM_QUALITY_LEGENDARY;
    default:
      return Color::ITEM_QUALITY_COMMON;
  }
}

bool ClientItem::canBeScrapped() const {
  return _class && _class->scrapping.canBeScrapped;
}

void ClientItem::fetchAmmoItem() const {
  if (_weaponAmmoID.empty()) return;

  auto it = _client->gameData.items.find(_weaponAmmoID);
  if (it == _client->gameData.items.end()) {
    _client->showErrorMessage(
        "Unknown item "s + _weaponAmmoID + " specified as ammo"s,
        Color::CHAT_ERROR);
    return;
  }
  _weaponAmmo = &(it->second);
}

void ClientItem::addParticles(const std::string &profileName,
                              const MapPoint &offset) {
  Particles p;
  p.profile = profileName;
  p.offset = offset;
  _particles.push_back(p);
}

const Tooltip &ClientItem::Instance::tooltip() const {
  auto shouldShowRepairTooltip = _type->_client->isAltPressed();
  if (shouldShowRepairTooltip) {
    if (!_repairTooltip.hasValue()) createRepairTooltip();
    return _repairTooltip.value();
  }

  if (!_tooltip.hasValue() || _tooltip.value().isDueForARefresh())
    createRegularTooltip();
  return _tooltip.value();
}

void ClientItem::Instance::createRegularTooltip() const {
  if (!_type) {
    _tooltip = Tooltip{};
    return;
  }

  _tooltip = _type->tooltip();
  auto &tooltip = _tooltip.value();

  tooltip.setColor(Color::TOOLTIP_BODY);

  if (_type->isQuestItem()) {
    tooltip.addGap();
    tooltip.addLine("Quest item");
  }

  if (isSoulbound()) {
    tooltip.addGap();
    tooltip.addLine("Soulbound");
  } else if (_type->bindsOnEquip()) {
    tooltip.addGap();
    tooltip.addLine("Binds when equipped");
  } else if (_type->bindsOnPickup()) {
    tooltip.addGap();
    tooltip.addLine("Binds when picked up");
  }

  if (_health == 0)
    tooltip.setColor(Color::DURABILITY_BROKEN);
  else if (_health <= 20)
    tooltip.setColor(Color::DURABILITY_LOW);

  if (!_type->_class) return;

  auto isDamaged = _health < Item::MAX_HEALTH;
  if (isDamaged) {
    auto oss = std::ostringstream{};
    oss << "Durability: "s << _health << "/"s << Item::MAX_HEALTH;
    tooltip.addGap();
    tooltip.addLine(oss.str());

    const auto needsRepairing = _health < Item::MAX_HEALTH;
    const auto canBeRepaired = _type->_class->repairing.canBeRepaired;
    if (canBeRepaired && needsRepairing) {
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine("Can be repaired (Alt-click).");
    }
  }

  if (_type->canBeScrapped()) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_BODY);
    tooltip.addLine("Can be scrapped (pick up + Del).");
  }
}

void ClientItem::Instance::createRepairTooltip() const {
  if (!_type->_class) {
    _repairTooltip = Tooltip::basicTooltip("This item cannot be repaired.");
    return;
  }
  const auto &repairInfo = _type->_class->repairing;

  if (!repairInfo.canBeRepaired) {
    _repairTooltip = Tooltip::basicTooltip("This item cannot be repaired.");
    return;
  }

  auto needsRepairing = _health < Item::MAX_HEALTH;
  if (!needsRepairing) {
    _repairTooltip =
        Tooltip::basicTooltip("This item is already at full health.");
    return;
  }

  _repairTooltip = Tooltip{};
  auto &rt = _repairTooltip.value();

  rt.setColor(Color::TOOLTIP_INSTRUCTION);
  rt.addLine("Alt-click to repair.");

  if (repairInfo.requiresTool()) {
    rt.addGap();
    rt.setColor(Color::TOOLTIP_BODY);
    rt.addLine("Requires tool:");
    rt.setColor(Color::TOOLTIP_TAG);
    rt.addLine(_type->_client->gameData.tagName(repairInfo.tool));
  }

  if (repairInfo.hasCost()) {
    rt.addGap();
    rt.setColor(Color::TOOLTIP_BODY);
    rt.addLine("Will consume:");
    const auto *costItem = _type->_client->findItem(repairInfo.cost);
    if (!costItem) return;
    rt.addItem(*costItem);
  }
}

bool ClientItem::Instance::isSoulbound() const { return _isSoulbound; }

std::string ClientItem::Instance::name() const {
  auto ret = _type->name();
  const auto hasSuffix = !_type->_suffixSet.empty();
  if (hasSuffix) {
    const auto suffixName =
        _type->_client->gameData.suffixSets.getSuffixName(_type->_suffixSet);
    ret += " of "s + suffixName;
  }
  return ret;
}
