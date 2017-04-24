#include "ClientCombatant.h"
#include "ClientCombatantType.h"

ClientCombatant::ClientCombatant(const ClientCombatantType *type):
_type(type),
_health(type->maxHealth())
{}

const Entity *ClientCombatant::asEntity() const {
    return reinterpret_cast<const Entity *>(this);
}
