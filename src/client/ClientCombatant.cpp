#include "ClientCombatant.h"
#include "ClientCombatantType.h"

ClientCombatant::ClientCombatant(const ClientCombatantType *type):
_type(type),
_health(type->maxHealth())
{}
