#include "ClientCombatantType.h"

ClientCombatantType::ClientCombatantType():
_maxHealth(0),
_damageParticles(nullptr)
{}

ClientCombatantType::ClientCombatantType(Hitpoints maxHealth):
_maxHealth(maxHealth),
_damageParticles(nullptr)
{}
