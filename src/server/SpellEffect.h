#pragma once

#include <map>

#include "../Podes.h"
#include "../SpellSchool.h"
#include "../combatTypes.h"
#include "combat.h"

class Entity;
class Spell;

class SpellEffect {
 public:
  // Each spell function should know if it needs any of these.
  struct Args {
    int i1{0};
    double d1{0};
    std::string s1, s2, s3;
  };

  void setFunction(const std::string &functionName);
  void args(const Args &args) { _args = args; }
  void range(Podes r) { _range = r.toPixels(); }
  void radius(Podes r) {
    _range = r.toPixels();
    _targetsInArea = true;
  }
  px_t range() const { return _range; }
  bool isAoE() const { return _targetsInArea; }
  void school(SpellSchool school) { _school = school; }
  SpellSchool school() const { return _school; }

  bool exists() const { return _function != nullptr; }
  bool isAggressive() const;

  CombatResult execute(Entity &caster, Entity &target,
                       const std::string &supplementaryArg = {}) const {
    return _function(*this, caster, target, supplementaryArg);
  }

  static Hitpoints chooseRandomSpellMagnitude(double raw);

 private:
  using Function = CombatResult (*)(const SpellEffect &effect, Entity &caster,
                                    Entity &target,
                                    const std::string &supplementaryArg);

  Function _function = nullptr;
  Args _args;
  px_t _range = Podes::MELEE_RANGE.toPixels();
  bool _targetsInArea = false;
  SpellSchool _school;

  using FunctionMap = std::map<std::string, Function>;
  static FunctionMap functionMap;

  using FlagMap = std::map<SpellEffect::Function, bool>;
  static FlagMap
      aggressionMap;  // Ultimately, whether a defense sound should play.

#define DECLARE_SPELL_FUNCTION(FUNC_NAME)                                  \
  static CombatResult FUNC_NAME(const SpellEffect &effect, Entity &caster, \
                                Entity &target,                            \
                                const std::string &supplementaryArg)

  DECLARE_SPELL_FUNCTION(doDirectDamage);
  DECLARE_SPELL_FUNCTION(doDirectDamageWithModifiedThreat);
  DECLARE_SPELL_FUNCTION(heal);
  DECLARE_SPELL_FUNCTION(scaleThreat);
  DECLARE_SPELL_FUNCTION(buff);
  DECLARE_SPELL_FUNCTION(debuff);
  DECLARE_SPELL_FUNCTION(dispellDebuff);
  DECLARE_SPELL_FUNCTION(randomTeleport);
  DECLARE_SPELL_FUNCTION(teleportToCity);
  DECLARE_SPELL_FUNCTION(teleportToHouse);
  DECLARE_SPELL_FUNCTION(teachRecipe);
  DECLARE_SPELL_FUNCTION(spawnNPC);
  DECLARE_SPELL_FUNCTION(randomBuff);
  DECLARE_SPELL_FUNCTION(playTwoUp);
};
