#include "ClientNPC.h"
#include "ClientNPCType.h"

ClientNPCType::ClientNPCType(const std::string &id, health_t maxHealth):
ClientObjectType(id),
_maxHealth(maxHealth)
{
    containerSlots(ClientNPC::LOOT_CAPACITY);
}
