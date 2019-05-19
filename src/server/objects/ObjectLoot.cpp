#include "ObjectLoot.h"

#include "../Server.h"
#include "Object.h"

ObjectLoot::ObjectLoot(Object &parent) : _parent(parent) {}

void ObjectLoot::populate() {
  addStrengthItemsToLoot();
  addContainerItemsToLoot();

  if (!empty()) _parent.sendAllLootToTagger();
}

void ObjectLoot::addStrengthItemsToLoot() {
  static const double MATERIAL_LOOT_CHANCE = 0.05;

  auto wasUnderConstruction = !_parent.remainingMaterials().isEmpty();
  if (wasUnderConstruction) return;

  const auto &strengthPair = _parent.objType().strengthPair();
  const ServerItem *strengthItem = strengthPair.first;
  size_t strengthQty = strengthPair.second;

  if (strengthItem == nullptr) return;

  size_t lootQuantity = 0;
  for (size_t i = 0; i != strengthQty; ++i)
    if (randDouble() < MATERIAL_LOOT_CHANCE) ++lootQuantity;

  add(strengthItem, lootQuantity);
}

void ObjectLoot::addContainerItemsToLoot() {
  static const double CONTAINER_LOOT_CHANCE = 0.05;
  if (_parent.hasContainer()) {
    auto lootFromContainer =
        _parent.container().generateLootWithChance(CONTAINER_LOOT_CHANCE);
    add(lootFromContainer);
  }
}
