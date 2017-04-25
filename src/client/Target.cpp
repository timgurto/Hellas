#include <cassert>

#include "Client.h"
#include "Target.h"

Target::Target() :
_entity(nullptr),
_combatant(nullptr),
_aggressive(false),
_panel(nullptr)
{}

void Target::setAndAlertServer(
        const Entity *asEntity, const ClientCombatant *asCombatant, bool nowAggressive){

    const Client &client = *Client::_instance;

    if (asEntity == nullptr){
        clearTarget();
        return;
    }

    const ClientCombatant &targetCombatant = *asCombatant;
    const Entity &targetEntity = *asEntity;

    if (! targetCombatant.canBeAttackedByPlayer())
        nowAggressive = false;

    if (targetIsDifferentFromServer(targetEntity, nowAggressive)){
        if (nowAggressive)
            targetCombatant.sendTargetMessage();
        else
            client.sendClearTargetMessage();
    }

    _entity = asEntity;
    _combatant = asCombatant;
    _aggressive = nowAggressive;

    _name = _entity->name();
    _health = _combatant->health();
    _maxHealth = _combatant->maxHealth();

    _panel->show();
}

void Target::clearTarget(){
    const Client &client = *Client::_instance;

    bool serverHasTarget = isAggressive();
    if (serverHasTarget)
        client.sendClearTargetMessage();
    clear();
}

bool Target::targetIsDifferentFromServer(const Entity &newTarget, bool nowAggressive){
    bool sameTargetAsBefore = &newTarget == _entity;
    bool aggressionLevelChanged = isAggressive() != nowAggressive;
    return sameTargetAsBefore || nowAggressive || aggressionLevelChanged;
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
