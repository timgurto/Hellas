#include "NPCType.h"
#include "NPC.h"
#include "Server.h"
#include "objects/Container.h"

Stats NPCType::BASE_STATS{};

NPCType::NPCType(const std::string &id) : ObjectType(id) {}

void NPCType::init() {
  BASE_STATS.maxHealth = 1;
  BASE_STATS.crit = 5;
  BASE_STATS.dodge = 5;
  BASE_STATS.speed = 70.0;
}

void NPCType::initialise() const {
  ObjectType::initialise();

  // Fetch known spells
  if (_knownSpellID.empty()) return;
  _knownSpell = Server::instance().findSpell(_knownSpellID);
  if (!_knownSpell) {
    Server::debug()("Skipping nonexistent NPC spell " + _knownSpellID,
                    Color::CHAT_ERROR);
  }
}

bool NPCType::canBeAttacked() const { return _aggression != NON_COMBATANT; }

bool NPCType::attacksNearby() const { return _aggression == AGGRESSIVE; }

void NPCType::addSimpleLoot(const ServerItem *item, double chance) {
  _lootTable.addSimpleItem(item, chance);
}

void NPCType::addNormalLoot(const ServerItem *item, double mean, double sd) {
  _lootTable.addNormalItem(item, mean, sd);
}
