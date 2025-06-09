#include "Target.h"

#include <cassert>

#include "Client.h"

Target::Target(const Client &client)
    : _client(client),
      _entity(nullptr),
      _combatant(nullptr),
      _aggressive(false),
      _panel(nullptr),
      _menu(nullptr) {}

void Target::setAndAlertServer(const Sprite &asEntity,
                               ClientCombatant &asCombatant,
                               bool nowAggressive) {
  ClientCombatant &targetCombatant = asCombatant;
  const Sprite &targetEntity = asEntity;

  if (!targetCombatant.canBeAttackedByPlayer()) nowAggressive = false;

  if (targetIsDifferentFromServer(targetEntity, nowAggressive)) {
    if (nowAggressive)
      targetCombatant.sendTargetMessage();
    else
      targetCombatant.sendSelectMessage();
  }

  _entity = &asEntity;
  _combatant = &asCombatant;
  _aggressive = nowAggressive;

  onTypeChange();

  _panel->show();
}

bool Target::targetIsDifferentFromServer(const Sprite &newTarget,
                                         bool nowAggressive) {
  bool sameTargetAsBefore = &newTarget == _entity;
  bool aggressionLevelChanged = isAggressive() != nowAggressive;
  return !sameTargetAsBefore || aggressionLevelChanged;
}

void Target::clear(const Client &client) {
  bool serverHasTarget = _entity != nullptr;
  if (serverHasTarget) client.sendClearTargetMessage();

  _entity = nullptr;
  _combatant = nullptr;
  _aggressive = false;

  _panel->hide();
  _menu->hide();
}

void Target::initializePanel(Client &client) {
  static const px_t X = CombatantPanel::STANDARD_WIDTH +
                        2 * CombatantPanel::GAP,
                    Y = CombatantPanel::GAP;
  _panel = new CombatantPanel(X, Y, CombatantPanel::TARGET_WIDTH, _name,
                              _health, _maxHealth, _energy, _maxEnergy, _level);
  _panel->hide();
  _panel->setRightMouseDownFunction(
      [this](Element &e, const ScreenPoint &mousePos) {
        openMenu(e, mousePos);
      },
      _menu);
  _panel->setClient(client);
  _menu->setClient(client);
}

void Target::initializeMenu() {
  static const px_t WIDTH = 120, ITEM_HEIGHT = 20;
  _menu = new List({_client.mouse().x, _client.mouse().y, WIDTH, 50});
  _menu->hide();
}

void Target::openMenu(Element &e, const ScreenPoint &) {
  List &menu = dynamic_cast<List &>(e);
  menu.setPosition(toInt(_client.mouse().x), toInt(_client.mouse().y));

  menu.clearChildren();

  menu.client()->targetAsCombatant()->addMenuButtons(menu);

  if (!menu.empty()) menu.show();
}

void Target::onTypeChange() {
  _name = _entity->name();
  _health = _combatant->health();
  _maxHealth = _combatant->maxHealth();
  _energy = _combatant->energy();
  _maxEnergy = _combatant->maxEnergy();
  _level = _combatant->level();

  _panel->setLevelColor(_level);

  _panel->setRank(_combatant->rank());

  if (_maxEnergy == 0)
    _panel->hideEnergyBar();
  else
    _panel->showEnergyBar();
}
