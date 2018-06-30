#include "CombatantPanel.h"
#include "ColorBlock.h"
#include "LinkedLabel.h"
#include "ShadowBox.h"

CombatantPanel::CombatantPanel(px_t panelX, px_t panelY,
                               const std::string &name, const Hitpoints &health,
                               const Hitpoints &maxHealth, const Energy &energy,
                               const Energy &maxEnergy)
    : Element({panelX, panelY, WIDTH, HEIGHT}) {
  addChild(new ColorBlock({0, 0, WIDTH, HEIGHT}));
  addChild(new ShadowBox({0, 0, WIDTH, HEIGHT}));

  const auto ELEMENT_WIDTH = WIDTH - GAP * 2;
  auto y = GAP;
  addChild(new LinkedLabel<std::string>(
      {GAP, y, ELEMENT_WIDTH, Element::TEXT_HEIGHT}, name, {}, {},
      Element::CENTER_JUSTIFIED));
  y += Element::TEXT_HEIGHT + GAP;

  _healthBar =
      new ProgressBar<Hitpoints>({GAP, y, ELEMENT_WIDTH, BAR_HEIGHT}, health,
                                 maxHealth, Color::COMBATANT_NEUTRAL);
  addChild(_healthBar);
  _healthBar->showValuesInTooltip();
  y += BAR_HEIGHT + GAP;

  _energyBar = new ProgressBar<Energy>({GAP, y, ELEMENT_WIDTH, BAR_HEIGHT},
                                       energy, maxEnergy, Color::ENERGY);
  _energyBar->showValuesInTooltip();
  addChild(_energyBar);
  y += BAR_HEIGHT + GAP;

  this->height(y);
}

void CombatantPanel::showEnergyBar() {
  if (!_energyBar->visible()) {
    _energyBar->show();
    height(height() + BAR_HEIGHT + GAP);
  }
}

void CombatantPanel::hideEnergyBar() {
  if (_energyBar->visible()) {
    _energyBar->hide();
    height(height() - BAR_HEIGHT - GAP);
  }
}
