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
  struct Args {
    int i1{0};
    double d1{0};
    std::string s1{};
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

  CombatResult execute(Entity &caster, Entity &target) const {
    return _function(*this, caster, target);
  }

  static Hitpoints chooseRandomSpellMagnitude(double raw);

 private:
  using Function = CombatResult (*)(const SpellEffect &effect, Entity &caster,
                                    Entity &target);

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

  static CombatResult doDirectDamage(const SpellEffect &effect, Entity &caster,
                                     Entity &target);
  static CombatResult doDirectDamageWithModifiedThreat(
      const SpellEffect &effect, Entity &caster, Entity &target);
  static CombatResult heal(const SpellEffect &effect, Entity &caster,
                           Entity &target);
  static CombatResult scaleThreat(const SpellEffect &effect, Entity &caster,
                                  Entity &target);
  static CombatResult buff(const SpellEffect &effect, Entity &caster,
                           Entity &target);
  static CombatResult debuff(const SpellEffect &effect, Entity &caster,
                             Entity &target);
  static CombatResult dispellDebuff(const SpellEffect &effect, Entity &caster,
                                    Entity &target);
  static CombatResult randomTeleport(const SpellEffect &effect, Entity &caster,
                                     Entity &target);
  static CombatResult teleportToCity(const SpellEffect &effect, Entity &caster,
                                     Entity &target);
};
