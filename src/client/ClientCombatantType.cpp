#include "ClientCombatantType.h"

ClientCombatantType::ClientCombatantType():
_maxHealth(1) // TODO: make this a 0 to force proper initialization
{}

ClientCombatantType::ClientCombatantType(health_t maxHealth):
_maxHealth(maxHealth)
{}
