#ifndef COMBATANT_PANEL_H
#define COMBATANT_PANEL_H

#include <string>

#include "../ClientCombatant.h"
#include "Element.h"
#include "ProgressBar.h"

// A panel displaying a combatant's health bar, name, etc.
class CombatantPanel : public Element {
 public:
  CombatantPanel(px_t x, px_t y, const std::string &name,
                 const Hitpoints &health, const Hitpoints &maxHealth,
                 const Energy &energy, const Energy &maxEnergy);

  void showEnergyBar();
  void hideEnergyBar();
  void addXPBar(const XP &xp, const XP &maxXP);

  static const px_t WIDTH = 100, HEIGHT = 40, BAR_HEIGHT = 7, GAP = 2;
  static const px_t ELEMENT_WIDTH = WIDTH - GAP * 2;

 private:
  ProgressBar<Hitpoints> *_healthBar;
  ProgressBar<Energy> *_energyBar;
  ProgressBar<XP> *_xpBar;
};

#endif
