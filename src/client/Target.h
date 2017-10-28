#ifndef TARGET_HEADER
#define TARGET_HEADER

#include <string>

#include "ClientCombatant.h"
#include "Sprite.h"
#include "ui/CombatantPanel.h"
#include "ui/List.h"
#include "../types.h"

class Target{
public:
    Target();

    template<typename T>
    void setAndAlertServer(const T &newTarget, bool nowAggressive){
        setAndAlertServer(newTarget, newTarget, nowAggressive);
    }

    void clear();

    const Sprite *entity() const { return _entity; }
    const ClientCombatant *combatant() const { return _combatant; }
    bool exists() const { return _entity != nullptr; }
    bool isAggressive() const { return _aggressive; }
    void makeAggressive() { _aggressive = true; }
    void makePassive() { _aggressive = false; }
    
    const std::string &name() const { return _name; }
    const Hitpoints &health() const { return _health; }
    const Hitpoints &maxHealth() const { return _maxHealth; }
    void refreshHealthBarColor();

    void updateHealth(Hitpoints newHealth){ _health = newHealth; }

    void initializePanel();
    CombatantPanel *panel() { return _panel; }
    void initializeMenu();
    List *menu() { return _menu; }

    static void openMenu(Element &e, const Point &mousePos);
    void hideMenu(){ _menu->hide(); }

private:
    /*
    Both pointers should contain the same value.  Having both is necessary because reinterpret_cast
    doesn't appear to work.
    */
    const Sprite *_entity;
    const ClientCombatant *_combatant;

    bool _aggressive; // True: will attack when in range.  False: mere selection, client-side only.

    /*
    Updated on set().  These are attributes, not functions, because the UI uses LinkedLabels that
    contain data references.
    */
    std::string _name;
    Hitpoints _health, _maxHealth;

    void setAndAlertServer(
            const Sprite &asEntity, const ClientCombatant &asCombatant, bool nowAggressive);
    bool targetIsDifferentFromServer(const Sprite &newTarget, bool nowAggressive);

    CombatantPanel *_panel;

    List *_menu;
};

#endif
