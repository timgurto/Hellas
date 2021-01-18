#pragma once

#include <map>
#include <vector>

#include "../Podes.h"
#include "../SpellSchool.h"
#include "../types.h"
#include "Entity.h"
#include "SpellEffect.h"

class Spell {
 public:
  enum TargetType {
    SELF,
    FRIENDLY,
    ENEMY,

    NUM_TARGET_TYPES
  };

  using ID = std::string;
  using Name = std::string;

  CombatResult performAction(Entity &caster, Entity &target,
                             const std::string &supplementaryArg) const;
  bool isTargetValid(const Entity &caster, const Entity &target) const;

  void id(const ID &id) { _id = id; }
  const ID &id() const { return _id; }
  void name(const Name &name) { _name = name; }
  const Name &name() const { return _name; }
  void setCanTarget(TargetType type) { _validTargets[type] = true; }
  bool canTarget(TargetType type) const { return _validTargets[type]; }
  void restrictTargetsToNPCsOfType(const std::string &npcTypeID) {
    _onlyAllowedNPCTarget = npcTypeID;
  }
  bool isTargetingRestrictedToSpecificNPC() const {
    return !_onlyAllowedNPCTarget.empty();
  }
  const std::string &onlyAllowedNPCTarget() const {
    return _onlyAllowedNPCTarget;
  }
  void cost(Energy e) { _cost = e; }
  Energy cost() const { return _cost; }
  bool shouldPlayDefenseSound() const { return _effect.isAggressive(); }
  void range(Podes r) { _range = r.toPixels(); }
  SpellEffect &effect() { return _effect; }
  const SpellEffect &effect() const { return _effect; }
  bool canCastOnlyOnSelf() const;
  void cooldown(ms_t cooldown) { _cooldown = cooldown; }
  ms_t cooldown() const { return _cooldown; }

 private:
  ID _id;
  Name _name;
  Energy _cost = 0;
  px_t _range = Podes::MELEE_RANGE.toPixels();
  ms_t _cooldown{0};

  SpellEffect _effect;

  using ValidTargets = std::vector<bool>;
  ValidTargets _validTargets = ValidTargets(NUM_TARGET_TYPES, false);
  std::string _onlyAllowedNPCTarget;  // Only NPCs of this type can be targeted.
};

using Spells = std::map<Spell::ID, Spell *>;
