#include "NPC.h"
#include "NPCType.h"
#include "objects/Container.h"

NPCType::NPCType(const std::string &id, Hitpoints maxHealth):
ObjectType(id),
_maxHealth(maxHealth),
_attack(0),
_attackTime(0)
{}

void NPCType::addSimpleLoot(const ServerItem * item, double chance) {
    _lootTable.addSimpleItem(item, chance);
}

void NPCType::addNormalLoot(const ServerItem *item, double mean, double sd){
    _lootTable.addNormalItem(item, mean, sd);
}
