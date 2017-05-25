#include "ClientCombatantType.h"

ClientCombatantType::ClientCombatantType():
_maxHealth(0),
_damageParticles(nullptr)
{}

ClientCombatantType::ClientCombatantType(health_t maxHealth):
_maxHealth(maxHealth),
_damageParticles(nullptr)
{}
