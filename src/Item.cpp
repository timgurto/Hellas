#include "Item.h"

Item::Item(const std::string &id) : _id(id) {}

Item::GearSlot Item::parseGearSlot(std::string slotName) {
  slotName = toLower(slotName);
  if (slotName == "head") return HEAD;
  if (slotName == "jewelry") return JEWELRY;
  if (slotName == "body") return BODY;
  if (slotName == "shoulders") return SHOULDERS;
  if (slotName == "hands") return HANDS;
  if (slotName == "feet") return FEET;
  if (slotName == "weapon") return WEAPON;
  if (slotName == "offhand") return OFFHAND;

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

void Item::initialiseMaxHealthFromIlvlAndQuality() {
  _maxHealth = _ilvl * 10 + _quality;
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
