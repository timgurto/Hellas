#include "Client.h"
#include "ClientNPC.h"
#include "ClientNPCType.h"
#include "Surface.h"

ClientNPCType::ClientNPCType(const std::string &id, health_t maxHealthArg):
ClientObjectType(id)
{
    containerSlots(ClientNPC::LOOT_CAPACITY);
    maxHealth(maxHealthArg);
}
