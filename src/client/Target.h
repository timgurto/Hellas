#ifndef TARGET_HEADER
#define TARGET_HEADER

#include <string>

#include "../types.h"
#include "ClientCombatant.h"
#include "Sprite.h"
#include "ui/CombatantPanel.h"
#include "ui/List.h"

class Target {
 public:
  Target();

  template <typename T>
  void setAndAlertServer(const T &newTarget, bool nowAggressive) {
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
  const Energy &energy() const { return _energy; }
  const Energy &maxEnergy() const { return _energy; }
  void refreshHealthBarColor();

  void updateHealth(Hitpoints newHealth) { _health = newHealth; }
  void updateEnergy(Energy newEnergy) { _energy = newEnergy; }

  void initializePanel();
  CombatantPanel *panel() { return _panel; }
  void initializeMenu();
  List *menu() { return _menu; }

  static void openMenu(Element &e, const ScreenPoint &mousePos);
  void hideMenu() { _menu->hide(); }

  void onTypeChange();

 private:
  /*
  Both pointers should contain the same value.  Having both is necessary because
  reinterpret_cast doesn't appear to work.
  */
  const Sprite *_entity;
  const ClientCombatant *_combatant;

  bool _aggressive;  // True: will attack when in range.  False: mere selection,
                     // client-side only.

  /*
  Updated on set().  These are attributes, not functions, because the UI uses
  LinkedLabels that contain data references.
  */
  std::string _name;
  Hitpoints _health, _maxHealth;
  Energy _energy, _maxEnergy;

  void setAndAlertServer(const Sprite &asEntity,
                         const ClientCombatant &asCombatant,
                         bool nowAggressive);
  bool targetIsDifferentFromServer(const Sprite &newTarget, bool nowAggressive);

  CombatantPanel *_panel;

  List *_menu;
};

#endif
