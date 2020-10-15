#ifndef COMBATANT_PANEL_H
#define COMBATANT_PANEL_H

#include <string>

#include "../ClientCombatant.h"
#include "Element.h"
#include "ProgressBar.h"

class ColorBlock;
class Picture;
class ShadowBox;

// A panel displaying a combatant's health bar, name, etc.
class CombatantPanel : public Element {
 public:
  CombatantPanel(px_t panelX, px_t panelY, px_t width, const std::string &name,
                 const Hitpoints &health, const Hitpoints &maxHealth,
                 const Energy &energy, const Energy &maxEnergy,
                 const Level &level);

  void showEnergyBar();
  void hideEnergyBar();
  void addXPBar(const XP &xp, const XP &maxXP);

  static const px_t STANDARD_WIDTH = 110, TARGET_WIDTH = 160, HEIGHT = 40,
                    BAR_HEIGHT = 7, GAP = 2, SPACE_FOR_LEVEL = 20;

  virtual void height(px_t h) override;
  void setRank(ClientCombatantType::Rank rank);

 private:
  ProgressBar<Hitpoints> *_healthBar;
  ProgressBar<Energy> *_energyBar;
  ProgressBar<XP> *_xpBar;

  ColorBlock *_background;
  ShadowBox *_outline;

  Picture *_eliteMarker{nullptr}, *_bossMarker{nullptr};

  const px_t ELEMENT_WIDTH;
};

#endif
