#include <cassert>

#include "Target.h"

Target::Target() :
_entity(nullptr),
_combatant(nullptr)
{}

void Target::set(const Entity &asEntity, const ClientCombatant &asCombatant){
    _entity = &asEntity;
    _combatant = &asCombatant;

    _name = _entity->name();
    _health = _combatant->health();
    _maxHealth = _combatant->maxHealth();
}

void Target::clear(){
    _entity = nullptr;
    _combatant = nullptr;
}
