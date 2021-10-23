#include "Item.h"

Item::Item(const std::string &id) : _id(id) {}

Item::GearSlot Item::parseGearSlot(std::string slotName) {
  slotName = toLower(slotName);
  if (slotName == "chassis") return CHASSIS;
  if (slotName == "camo") return CAMO;
  if (slotName == "wheels") return WHEELS;
  if (slotName == "gun") return GUN;

  return NOT_GEAR;
}

void Item::makeWeapon(Hitpoints damage, double speedInS, SpellSchool school) {
  _stats.weaponDamage = damage;
  _stats.attackTime = static_cast<ms_t>(speedInS * 1000 + 0.5);
  _stats.weaponSchool = school;
}

void Item::setBinding(std::string mode) {
  if (mode == "pickup")
    _soulbinding = BIND_ON_PICKUP;
  else if (mode == "equip")
    _soulbinding = BIND_ON_EQUIP;
}

void Item::initialiseLvlReq() {
  if (isGear()) return;

  switch (_quality) {
    case COMMON:
      _lvlReq = _ilvl - 5;
      break;
    case UNCOMMON:
      _lvlReq = _ilvl - 10;
      break;
    case RARE:
      _lvlReq = _ilvl - 15;
      break;
    case EPIC:
      _lvlReq = _ilvl - 20;
      break;
    case LEGENDARY:
      _lvlReq = _ilvl - 25;
      break;
  }
  if (_lvlReq < 0) _lvlReq = 0;
}

static double qualityMultiplierForItemHealth(Item::Quality quality) {
  const auto QUALITY_EXPONENT = 1.1;
  const auto Q0 = 1.0;
  const auto Q1 = QUALITY_EXPONENT;
  const auto Q2 = Q1 * QUALITY_EXPONENT;
  const auto Q3 = Q2 * QUALITY_EXPONENT;
  const auto Q4 = Q3 * QUALITY_EXPONENT;
  switch (quality) {
    case Item::COMMON:
      return Q0;
    case Item::UNCOMMON:
      return Q1;
    case Item::RARE:
      return Q2;
    case Item::EPIC:
      return Q3;
    case Item::LEGENDARY:
      return Q4;
    default:
      return 1;
  }
}

void Item::initialiseMaxHealthFromIlvlAndQuality() {
  const auto base = 3;
  const auto qualityMultiplier = qualityMultiplierForItemHealth(_quality);
  auto raw = base + qualityMultiplier * _ilvl;

  _maxHealth = toInt(raw);
}

bool Item::canBeDamaged() const {
  if (hasTags())
    return _ilvl > 0;  // Tools can always be damaged.
                       // The assumption here is that items without an explicit
                       // ilvl cannot be tools. Food is a good example here: has
                       // a tag, but no ilvl.

  if (!isGear()) return false;
  if (_weaponAmmo == this) return false;  // Thrown weapon; consumes itself

  return true;
}

Item::GearSlot Item::getRandomArmorSlot() {
  const auto choice = rand() % 6;
  switch (choice) {
    case 0:
      return HEAD;
    case 1:
      return BODY;
    case 2:
      return SHOULDERS;
    case 3:
      return HANDS;
    case 4:
      return FEET;
    case 5:
    default:
      return OFFHAND;
  }
}
