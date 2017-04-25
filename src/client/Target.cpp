#include <cassert>

#include "Client.h"
#include "Target.h"

Target::Target() :
_entity(nullptr),
_combatant(nullptr),
_aggressive(false),
_panel(nullptr)
{}

void Target::set(const Entity &asEntity, const ClientCombatant &asCombatant,
                                bool nowAggressive){
    _entity = &asEntity;
    _combatant = &asCombatant;
    _aggressive = nowAggressive;

    _name = _entity->name();
    _health = _combatant->health();
    _maxHealth = _combatant->maxHealth();

    _panel->show();
}

void Target::clear(){
    _entity = nullptr;
    _combatant = nullptr;
    _aggressive = false;

    _panel->hide();
}

void Target::initializePanel(){
    static const px_t
        X = CombatantPanel::WIDTH + 2 * CombatantPanel::GAP,
        Y = CombatantPanel::GAP;
    _panel = new CombatantPanel(X, Y, _name, _health, _maxHealth);
}
