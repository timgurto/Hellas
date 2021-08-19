#include "NPCType.h"

#include "NPC.h"
#include "Server.h"
#include "objects/Container.h"

Stats NPCType::BASE_STATS{};

NPCType::NPCType(const std::string &id) : ObjectType(id) {}

void NPCType::init() {
  BASE_STATS.maxHealth = 1;
  BASE_STATS.crit = 500;
  BASE_STATS.dodge = 500;
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

void NPCType::applyTemplate(const std::string &templateID) {
  auto nt = Server::instance().findNPCTemplate(templateID);
  if (!nt) return;
  collisionRect(*nt);
}

bool NPCType::canBeAttacked() const { return _aggression != NON_COMBATANT; }

bool NPCType::attacksNearby() const { return _aggression == AGGRESSIVE; }

void NPCType::addLootTable(const LootTable &rhs) { _lootTable.addAllFrom(rhs); }

void NPCType::addLootChoice(
    const std::vector<std::pair<const ServerItem *, int>> &choices) {
  _lootTable.addChoiceOfItems(choices);
}
