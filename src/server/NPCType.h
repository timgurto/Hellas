#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include "LootTable.h"
#include "QuestNode.h"
#include "Spell.h"
#include "objects/ObjectType.h"

class ClientItem;

// Describes a class of NPCs, a la the ObjectType class.
class NPCType : public ObjectType {
 public:
  enum Aggression {
    AGGRESSIVE,     // Will attack nearby enemies
    NEUTRAL,        // Will defend itself
    NON_COMBATANT,  // Cannot be attacked
  };

  LootTable _lootTable;
  Level _level;
  bool _isRanged{false};
  SpellSchool _school{SpellSchool::PHYSICAL};
  Aggression _aggression{AGGRESSIVE};
  Spell::ID _knownSpellID;
  mutable const Spell *_knownSpell{nullptr};

 public:
  static Stats BASE_STATS;

  NPCType(const std::string &id);
  virtual ~NPCType() {}

  static void init();
  virtual void initialise() const override;

  const LootTable &lootTable() const { return _lootTable; }
  void level(Level l) { _level = l; }
  Level level() const { return _level; }
  void makeRanged() { _isRanged = true; }
  bool isRanged() const { return _isRanged; }
  void makeCivilian() { _aggression = NON_COMBATANT; }
  void makeNeutral() { _aggression = NEUTRAL; }
  bool canBeAttacked() const;
  bool attacksNearby() const;
  void school(SpellSchool school) { _school = school; }
  SpellSchool school() const { return _school; }
  void knowsSpell(const Spell::ID &spell) { _knownSpellID = spell; }
  const Spell *knownSpell() const { return _knownSpell; }

  virtual char classTag() const override { return 'n'; }

  void addSimpleLoot(const ServerItem *item, double chance);
  void addNormalLoot(const ServerItem *item, double mean, double sd);
};

#endif
