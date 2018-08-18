#pragma once

#include <map>
#include <set>
#include <string>

#include "../Stats.h"
#include "SpellEffect.h"

class BuffType {
 public:
  enum Type {
    UNKNOWN,

    STATS,
    SPELL_OVER_TIME,
    SPELL_ON_HIT
  };

  using ID = std::string;

  BuffType() {}
  BuffType(const ID &id) : _id(id) {}

  const ID &id() const { return _id; }

  void stats(const StatsMod &stats);
  const StatsMod &stats() const { return _stats; }
  SpellSchool school() const { return _effect.school(); }

  SpellEffect &effect() { return _effect; }
  void effectOverTime() { _type = SPELL_OVER_TIME; }
  void effectOnHit() { _type = SPELL_ON_HIT; }
  const SpellEffect &effect() const { return _effect; }
  void tickTime(ms_t t) { _tickTime = t; }
  ms_t tickTime() const { return _tickTime; }
  void duration(ms_t t) { _duration = t; }
  ms_t duration() const { return _duration; }
  void makeInterruptible() { _canBeInterrupted = true; }
  bool canBeInterrupted() const { return _canBeInterrupted; }

  bool hasType() const { return _type != UNKNOWN; }
  bool hasEffectOnHit() const { return _type == SPELL_ON_HIT; }

 private:
  ID _id{};
  Type _type = UNKNOWN;

  StatsMod _stats{};

  SpellEffect _effect{};
  ms_t _tickTime{0};
  ms_t _duration{0};              // 0: Never ends
  bool _canBeInterrupted{false};  // Cancels when hit
};

// An instance of a buff type, on a specific target, from a specific caster
class Buff {
 public:
  using ID = std::string;

  Buff(const BuffType &type, Entity &owner, Entity &caster);
  Buff(const BuffType &type, Entity &owner, ms_t timeRemaining);

  const ID &type() const { return _type->id(); }
  bool hasExpired() const { return _expired; }
  SpellSchool school() const { return _type->school(); }
  bool hasEffectOnHit() const { return _type->hasEffectOnHit(); }
  bool canBeInterrupted() const { return _type->canBeInterrupted(); }

  bool operator==(const Buff &rhs) const { return _type == rhs._type; }

  void applyStatsTo(Stats &stats) const { stats &= _type->stats(); }

  void clearCasterIfEqualTo(const Entity &casterToRemove) const;

  ms_t timeRemaining() const { return _timeRemaining; }

  void update(ms_t timeElapsed);

  void proc(Entity *target = nullptr) const;  // Default: buff owner is target

 private:
  const BuffType *_type = nullptr;
  Entity *_owner = nullptr;

  // Always initialized for new buffs, but can become null if caster disappears.
  // When loaded from XML (User logged off with buff), this is null.
  mutable Entity *_caster = nullptr;

  ms_t _timeSinceLastProc{0};
  ms_t _timeRemaining{0};

  bool _expired{false};  // A signal that the buff should be removed before the
                         // next update()
};

using BuffTypes = std::map<Buff::ID, BuffType>;
using Buffs = std::vector<Buff>;
