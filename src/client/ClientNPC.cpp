#include "ClientNPC.h"

#include <cassert>

#include "Client.h"

extern Renderer renderer;

ClientNPC::ClientNPC(Serial serial, const ClientNPCType *type,
                     const MapPoint &loc)
    : ClientObject(serial, type, loc) {}

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
    const auto &client = *Client::_instance;
    return client.isAtWarWithObjectOwner(_owner);
  }
  return true;
}

const Color &ClientNPC::nameColor() const {
  if (canBeAttackedByPlayer()) {
    return npcType()->isNeutral() ? Color::COMBATANT_DEFENSIVE
                                  : Color::COMBATANT_ENEMY;
  }
  if (belongsToPlayerCity()) return Color::COMBATANT_ALLY;
  if (belongsToPlayer()) return Color::COMBATANT_SELF;
  return Color::COMBATANT_NEUTRAL;
}

void ClientNPC::update(double delta) {
  auto &client = Client::instance();

  auto shouldDrawGear = isAlive() && npcType()->hasGear();
  if (shouldDrawGear)
    client.drawGearParticles(npcType()->gear(), location(), delta);

  ClientObject::update(delta);
}

void ClientNPC::draw(const Client &client) const {
  ClientObject::draw(client);

  auto shouldDrawGear = isAlive() && npcType()->hasGear();
  if (shouldDrawGear) {
    const auto screenOffset = Client::_instance->offset();
    for (const auto &pair : ClientItem::drawOrder()) {
      const ClientItem *item = npcType()->gear(pair.second);
      if (item) item->draw(toScreenPoint(location()) + screenOffset);
    }
  }

  drawBuffEffects(location());
}

bool ClientNPC::shouldDrawName() const {
  if (isDead()) return false;
  return true;
}
