#include "NPC.h"
#include "NPCType.h"

NPCType::NPCType(const std::string &id, health_t maxHealth):
ObjectType(id),
_maxHealth(maxHealth)
{
    containerSlots(NPC::LOOT_CAPACITY);
}

void NPCType::addLoot(const ServerItem *item, double mean, double sd){
    _lootTable.addItem(item, mean, sd);
}
