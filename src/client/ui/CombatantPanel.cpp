#include "CombatantPanel.h"

#include "../Client.h"
#include "ColorBlock.h"
#include "LinkedLabel.h"
#include "Picture.h"
#include "ShadowBox.h"

CombatantPanel::CombatantPanel(px_t panelX, px_t panelY, px_t width,
                               const std::string &name, const Hitpoints &health,
                               const Hitpoints &maxHealth, const Energy &energy,
                               const Energy &maxEnergy, const Level &level)
    : Element({panelX, panelY, width, HEIGHT}), ELEMENT_WIDTH(width - GAP * 2) {
  _background = new ColorBlock({0, 0, width, HEIGHT});
  addChild(_background);

  _outline = new ShadowBox({0, 0, width, HEIGHT});
  addChild(_outline);

  auto y = GAP;

  const auto &wreath = Client::images.eliteWreath;
  _eliteMarker =
      new Picture{GAP * 2 + (SPACE_FOR_LEVEL - wreath.width()) / 2,
                  y + (Element::TEXT_HEIGHT - wreath.height()) / 2, wreath};
  addChild(_eliteMarker);
  _eliteMarker->hide();

  _bossMarker = new Picture{_eliteMarker->rect(), Client::images.bossWreath};
  addChild(_bossMarker);
  _bossMarker->hide();

  addChild(new LinkedLabel<Level>{
      {GAP * 2, y, SPACE_FOR_LEVEL, Element::TEXT_HEIGHT},
      level,
      {},
      {},
      Element::CENTER_JUSTIFIED});

  addChild(new LinkedLabel<std::string>(
      {GAP, y, ELEMENT_WIDTH, Element::TEXT_HEIGHT}, name, {}, {},
      Element::CENTER_JUSTIFIED));
  y += Element::TEXT_HEIGHT + GAP;

  _healthBar =
      new ProgressBar<Hitpoints>({GAP, y, ELEMENT_WIDTH, BAR_HEIGHT}, health,
                                 maxHealth, Color::STAT_HEALTH);
  addChild(_healthBar);
  _healthBar->showValuesInTooltip(" health");
  y += BAR_HEIGHT + GAP;

  _energyBar = new ProgressBar<Energy>({GAP, y, ELEMENT_WIDTH, BAR_HEIGHT},
                                       energy, maxEnergy, Color::STAT_ENERGY);
  _energyBar->showValuesInTooltip(" energy");
  addChild(_energyBar);
  y += BAR_HEIGHT + GAP;

  this->height(y);
}

void CombatantPanel::showEnergyBar() {
  if (!_energyBar->visible()) {
    _energyBar->show();
    height(Element::height() + BAR_HEIGHT + GAP);
  }
}

void CombatantPanel::hideEnergyBar() {
  if (_energyBar->visible()) {
    _energyBar->hide();
    height(Element::height() - BAR_HEIGHT - GAP);
  }
}

void CombatantPanel::addXPBar(const XP &xp, const XP &maxXP) {
  _xpBar = new ProgressBar<Energy>(
      {GAP, Element::height(), ELEMENT_WIDTH, BAR_HEIGHT}, xp, maxXP,
      Color::STAT_XP);
  height(Element::height() + BAR_HEIGHT + GAP);
  _xpBar->showValuesInTooltip(" experience");
  addChild(_xpBar);
}

void CombatantPanel::height(px_t h) {
  Element::height(h);
  _background->height(h);
  _outline->height(h);
}

void CombatantPanel::setRank(ClientCombatantType::Rank rank) {
  if (rank == ClientCombatantType::ELITE) {
    _eliteMarker->show();
    _bossMarker->hide();
  } else if (rank == ClientCombatantType::BOSS) {
    _eliteMarker->hide();
    _bossMarker->show();
  } else {
    _eliteMarker->hide();
    _bossMarker->hide();
  }
}
