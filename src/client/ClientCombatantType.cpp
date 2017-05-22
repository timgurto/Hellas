#include "ClientCombatantType.h"

ClientCombatantType::ClientCombatantType():
_maxHealth(0)
{}

ClientCombatantType::ClientCombatantType(health_t maxHealth):
_maxHealth(maxHealth)
{}
