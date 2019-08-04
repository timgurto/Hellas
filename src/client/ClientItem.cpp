#include "ClientItem.h"

#include "../XmlReader.h"
#include "Client.h"
#include "Tooltip.h"
#include "Unlocks.h"

std::map<int, size_t> ClientItem::gearDrawOrder;
std::vector<ScreenPoint> ClientItem::gearOffsets(Client::GEAR_SLOTS);

ClientItem::ClientItem(const std::string &id, const std::string &name)
    : Item(id), _name(name), _constructsObject(nullptr), _sounds(nullptr) {}

void ClientItem::icon(const std::string &filename) {
  static const std::string prefix = "Images/Items/", suffix = ".png";
  _icon = {prefix + filename + suffix};

  if (!_icon) _icon = {prefix + "none" + suffix};
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

void ClientItem::draw(const MapPoint &loc) const {
  if (_gearSlot <= Client::GEAR_SLOTS && _gearImage) {
    ScreenPoint drawLoc =
        _drawLoc +                    // The item's offset
        gearOffsets[_gearSlot] +      // The slot's offset
        toScreenPoint(loc) +          // The avatar's location
        Client::_instance->offset();  // The overall map offset
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

  const auto &client = *Client::_instance;

  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();

  tooltip.setColor(Color::TOOLTIP_NAME);
  tooltip.addLine(_name);

  // Gear slot/stats
  if (_gearSlot != Client::GEAR_SLOTS) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_BODY);
    tooltip.addLine("Gear: "s + Client::GEAR_SLOT_NAMES[_gearSlot]);

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
  if (hasTags()) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_TAG);
    for (const std::string &tag : tags()) tooltip.addLine(client.tagName(tag));
  }

  // Construction
  if (_constructsObject != nullptr) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
    tooltip.addLine(std::string("Right-click to place ") +
                    _constructsObject->name());
    if (!_constructsObject->constructionReq().empty())
      tooltip.addLine("(Requires " +
                      client.tagName(_constructsObject->constructionReq()) +
                      ")");

    // Vehicle?
    if (_constructsObject->classTag() == 'v') {
      tooltip.setColor(Color::TOOLTIP_BODY);
      tooltip.addLine("  Vehicle");
    }

    if (_constructsObject->containerSlots() > 0) {
      tooltip.setColor(Color::TOOLTIP_BODY);
      tooltip.addLine("  Container: " +
                      toString(_constructsObject->containerSlots()) + " slots");
    }

    if (_constructsObject->merchantSlots() > 0) {
      tooltip.setColor(Color::TOOLTIP_BODY);
      tooltip.addLine("  Merchant: " +
                      toString(_constructsObject->merchantSlots()) + " slots");
    }

    // Tags
    if (_constructsObject->hasTags()) {
      tooltip.setColor(Color::TOOLTIP_TAG);
      for (const std::string &tag : _constructsObject->tags())
        tooltip.addLine("  " + client.tagName(tag));
    }
  }

  // Spell
  if (castsSpellOnUse()) {
    auto it = client._spells.find(spellToCastOnUse());
    if (it == client._spells.end()) {
      client.showErrorMessage("Can't find spell: "s + spellToCastOnUse(),
                              Color::CHAT_ERROR);
    } else {
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine("Right-click: "s + it->second->createEffectDescription());
    }
  }

  // Unlocks
  auto acquireInfo = Unlocks::getEffectInfo({Unlocks::ACQUIRE, id()});
  auto gatherInfo = Unlocks::getEffectInfo({Unlocks::GATHER, id()});
  if (acquireInfo.hasEffect || gatherInfo.hasEffect) tooltip.addGap();
  if (acquireInfo.hasEffect) {
    tooltip.setColor(acquireInfo.color);
    tooltip.addLine(acquireInfo.message);
  }
  if (gatherInfo.hasEffect) {
    tooltip.setColor(gatherInfo.color);
    tooltip.addLine(gatherInfo.message);
  }

  return tooltip;
}

void ClientItem::sounds(const std::string &id) {
  const Client &client = *Client::_instance;
  _sounds = client.findSoundProfile(id);
}

bool ClientItem::canUse() const {
  return _constructsObject != nullptr || castsSpellOnUse();
}

void ClientItem::fetchAmmoItem() const {
  if (_weaponAmmoID.empty()) return;

  const Client &client = *Client::_instance;
  auto it = client._items.find(_weaponAmmoID);
  if (it == client._items.end()) {
    client.showErrorMessage(
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
  auto shouldShowRepairTooltip = Client::instance().isAltPressed();
  if (shouldShowRepairTooltip) {
    if (!_repairTooltip.hasValue()) createRepairTooltip();
    return _repairTooltip.value();
  }

  if (!_tooltip.hasValue()) createRegularTooltip();
  return _tooltip.value();
}

void ClientItem::Instance::createRegularTooltip() const {
  if (!_type) {
    _tooltip = Tooltip{};
    return;
  }

  _tooltip = _type->tooltip();
  auto &tooltip = _tooltip.value();

  auto color = Color::TOOLTIP_BODY;
  if (_health == 0)
    color = Color::DURABILITY_BROKEN;
  else if (_health <= 20)
    color = Color::DURABILITY_LOW;
  tooltip.setColor(color);

  // Removed while item damage is temporarily disabled
  // auto oss = std::ostringstream{};
  // oss << "Durability: "s << _health << "/"s << Item::MAX_HEALTH;
  // tooltip.addGap();
  // tooltip.addLine(oss.str());
  // auto needsRepairing = _health < Item::MAX_HEALTH;
  // if (_type->repairInfo().canBeRepaired && needsRepairing) {
  //   tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
  //   tooltip.addLine("Alt-click to repair");
  // }
}

void ClientItem::Instance::createRepairTooltip() const {
  if (!_type->repairInfo().canBeRepaired) {
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

  if (_type->repairInfo().requiresTool()) {
    rt.addGap();
    rt.setColor(Color::TOOLTIP_BODY);
    rt.addLine("Requires tool:");
    rt.setColor(Color::TOOLTIP_TAG);
    rt.addLine(Client::instance().tagName(_type->repairInfo().tool));
  }

  if (_type->repairInfo().hasCost()) {
    rt.addGap();
    rt.setColor(Color::TOOLTIP_BODY);
    rt.addLine("Will consume:");
    const auto *costItem =
        Client::instance().findItem(_type->repairInfo().cost);
    if (!costItem) return;
    rt.addItem(*costItem);
  }

  return;
}
