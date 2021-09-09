#include "ObjectLoot.h"

#include "../Server.h"
#include "Object.h"

ObjectLoot::ObjectLoot(Object &parent) : _parent(parent) {}

void ObjectLoot::populate() {
  addContainerItemsToLoot();

  if (!empty()) _parent.sendAllLootToTaggers();
}

void ObjectLoot::addContainerItemsToLoot() {
  static const double CONTAINER_LOOT_CHANCE = 0.05;
  if (_parent.hasContainer()) {
    auto lootFromContainer =
        _parent.container().generateLootWithChance(CONTAINER_LOOT_CHANCE);
    add(lootFromContainer);
  }
}
