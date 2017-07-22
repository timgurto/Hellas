#include <cassert>

#include "Client.h"
#include "Target.h"

Target::Target() :
_entity(nullptr),
_combatant(nullptr),
_aggressive(false),
_panel(nullptr),
_menu(nullptr)
{}

void Target::setAndAlertServer(
        const Sprite &asEntity, const ClientCombatant &asCombatant, bool nowAggressive){
    const Client &client = *Client::_instance;
    const ClientCombatant &targetCombatant = asCombatant;
    const Sprite &targetEntity = asEntity;

    if (! targetCombatant.canBeAttackedByPlayer())
        nowAggressive = false;

    if (targetIsDifferentFromServer(targetEntity, nowAggressive)){
        if (nowAggressive)
            targetCombatant.sendTargetMessage();
        else
            client.sendClearTargetMessage();
    }

    _entity = &asEntity;
    _combatant = &asCombatant;
    _aggressive = nowAggressive;

    _name = _entity->name();
    _health = _combatant->health();
    _maxHealth = _combatant->maxHealth();
    refreshHealthBarColor();

    _panel->show();
}

void Target::refreshHealthBarColor(){
    if (_combatant == nullptr)
        return;
    _panel->changeColor(_combatant->nameColor());
}

bool Target::targetIsDifferentFromServer(const Sprite &newTarget, bool nowAggressive){
    bool sameTargetAsBefore = &newTarget == _entity;
    bool aggressionLevelChanged = isAggressive() != nowAggressive;
    return !sameTargetAsBefore || nowAggressive || aggressionLevelChanged;
}

void Target::clear(){
    const Client &client = *Client::_instance;

    bool serverHasTarget = isAggressive();
    if (serverHasTarget)
        client.sendClearTargetMessage();

    _entity = nullptr;
    _combatant = nullptr;
    _aggressive = false;

    _panel->hide();
    _menu->hide();
}

void Target::initializePanel(){
    static const px_t
        X = CombatantPanel::WIDTH + 2 * CombatantPanel::GAP,
        Y = CombatantPanel::GAP;
    _panel = new CombatantPanel(X, Y, _name, _health, _maxHealth);
    _panel->hide();
    _panel->setRightMouseDownFunction(openMenu, _menu);
}

void Target::initializeMenu(){
    static const px_t
        WIDTH = 80,
        ITEM_HEIGHT = 15;
    _menu = new List(Rect(_menu->absMouse->x, _menu->absMouse->y, WIDTH, 50));
    _menu->hide();
}

void Target::openMenu(Element &e, const Point &mousePos){
    List &menu = dynamic_cast<List &>(e);
    menu.setPosition(
            toInt(menu.absMouse->x),
            toInt(menu.absMouse->y));

    menu.clearChildren();
    
    Client &client = *Client::_instance;
    client.targetAsCombatant()->addMenuButtons(menu);

    if (!menu.empty())
        menu.show();

}
