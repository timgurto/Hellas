#include "ClientNPC.h"

#include <cassert>

#include "Client.h"

extern Renderer renderer;

ClientNPC::ClientNPC(Client &client, Serial serial, const ClientNPCType *type,
                     const MapPoint &loc)
    : ClientObject(serial, type, loc, client) {}

bool ClientNPC::canBeTamed() const {
  if (!npcType()->canBeTamed()) return false;
  if (!isAlive()) return false;
  if (owner().type != Owner::ALL_HAVE_ACCESS) return false;
  return true;
}

double ClientNPC::getTameChance() const {
  return getTameChanceBasedOnHealthPercent(1.0 * health() / maxHealth());
}

bool ClientNPC::canBeAttackedByPlayer() const {
  if (!ClientCombatant::canBeAttackedByPlayer()) return false;
  if (npcType()->isCivilian()) return false;

  if (_owner.type != Owner::ALL_HAVE_ACCESS) {
    return _client.isAtWarWithObjectOwner(_owner);
  }
  return true;
}

Color ClientNPC::nameColor() const {
  if (canBeAttackedByPlayer()) {
    return npcType()->isNeutral() ? Color::COMBATANT_DEFENSIVE
                                  : Color::COMBATANT_ENEMY;
  }
  if (belongsToPlayerCity()) return Color::COMBATANT_ALLY;
  if (belongsToPlayer()) return Color::COMBATANT_SELF;
  return Color::COMBATANT_NEUTRAL;
}

void ClientNPC::update(double delta) {
  auto shouldDrawGear = isAlive() && npcType()->hasGear();
  if (shouldDrawGear)
    _client.drawGearParticles(npcType()->gear(), location(), delta);

  ClientObject::update(delta);
}

void ClientNPC::draw() const {
  ClientObject::draw();

  auto shouldDrawGear = isAlive() && npcType()->hasGear();
  if (shouldDrawGear) {
    const auto screenOffset = _client.offset();
    for (const auto &pair : ClientItem::drawOrder()) {
      const ClientItem *item = npcType()->gear(pair.second);
      if (item) item->draw(toScreenPoint(location()) + screenOffset);
    }
  }

  drawBuffEffects(location(), _client.offset());
}

bool ClientNPC::shouldDrawName() const {
  if (isDead()) return false;
  if (this->owner().type == Owner::CITY || this->owner().type == Owner::PLAYER)
    return ClientObject::shouldDrawName();
  return true;
}

bool ClientNPC::addClassSpecificStuffToWindow() {
  if (this->owner().type == Owner::ALL_HAVE_ACCESS) return false;
  if (!userHasAccess()) return false;

  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;

  auto *followButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Follow", [this]() {
        _client.sendMessage({CL_ORDER_NPC_TO_FOLLOW, serial()});
      });
  followButton->setTooltip("Order this pet to follow you.");
  _window->addChild(followButton);
  x += BUTTON_GAP + BUTTON_WIDTH;

  auto *stayButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Stay", [this]() {
        _client.sendMessage({CL_ORDER_NPC_TO_STAY, serial()});
      });
  stayButton->setTooltip("Order this pet to stay.");
  _window->addChild(stayButton);
  x += BUTTON_GAP + BUTTON_WIDTH;
  y += BUTTON_GAP + BUTTON_HEIGHT;
  if (newWidth < x) newWidth = x;

  x = BUTTON_GAP;
  auto *feedButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Feed", [this]() {
        _client.sendMessage({CL_FEED_PET, serial()});
      });
  feedButton->setTooltip(
      "Feed this pet to heal it.  This will consume food from your inventory.");
  _window->addChild(feedButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;

  _window->resize(newWidth, y);

  return true;
}
