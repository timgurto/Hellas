#include "Item.h"

Item::Item(const std::string &id) : _id(id), _durability(0) {}

void Item::makeWeapon(Hitpoints damage, double speedInS, SpellSchool school) {
  _stats.weaponDamage = damage;
  _stats.attackTime = static_cast<ms_t>(speedInS * 1000 + 0.5);
  _stats.weaponSchool = school;
}

void Item::addTag(const std::string &tagName, double toolSpeed) {
  _tags[tagName] = toolSpeed;
}

bool Item::isTag(const std::string &tagName) const {
  return _tags.find(tagName) != _tags.end();
}

size_t Item::getRandomArmorSlot() {
  /*
  Randomly return one of:
  0: head
  1: body
  3: shoulders
  4: hands
  5: feet
  7: left hand
  */
  size_t slot = rand() % 6;
  if (slot == 5) return 7;
  if (slot >= 1) return slot + 1;
  return slot;
}
