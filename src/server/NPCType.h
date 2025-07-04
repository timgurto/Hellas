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
  static const px_t DEFAULT_MAX_DISTANCE_FROM_HOME{500};
  px_t _maxDistanceFromHome{DEFAULT_MAX_DISTANCE_FROM_HOME};
  bool _pursuesEndlessly{false};  // Expected to be used only by tests.

  struct KnownSpell {
    std::string id;
    mutable const Spell *spell;
    bool canCastOutOfCombat;
  };
  using KnownSpells = std::vector<KnownSpell>;
  KnownSpells _knownSpells;

  bool _canBeTamed{false};
  std::string _tamingRequiresItem;

 public:
  static Stats BASE_STATS;

  NPCType(const std::string &id);
  virtual ~NPCType() {}

  static void init();
  virtual void initialise() const override;

  void applyTemplate(const std::string &templateID);

  const LootTable &lootTable() const { return _lootTable; }
  LootTable &lootTable() { return _lootTable; }
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
  void knowsSpell(Spell::ID id, bool canCastOutOfCombat) {
    _knownSpells.push_back({id, nullptr, canCastOutOfCombat});
  }
  const KnownSpells &knownSpells() const { return _knownSpells; }
  px_t maxDistanceFromHome() const { return _maxDistanceFromHome; }
  void maxDistanceFromHome(px_t dist) { _maxDistanceFromHome = dist; }
  void canBeTamed(bool b) { _canBeTamed = b; }
  bool canBeTamed() const { return _canBeTamed; }
  void tamingRequiresItem(const std::string &id) { _tamingRequiresItem = id; }
  const std::string &tamingRequiresItem() const { return _tamingRequiresItem; }
  void pursuesEndlessly(bool b) { _pursuesEndlessly = b; }
  bool pursuesEndlessly() const { return _pursuesEndlessly; }

  virtual char classTag() const override { return 'n'; }

  void addLootTable(const LootTable &rhs);
  void addLootChoice(
      const std::vector<std::pair<const ServerItem *, int>> &choices);
};

#endif
