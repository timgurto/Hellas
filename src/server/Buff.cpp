#include "Buff.h"

#include "Server.h"

void BuffType::stats(const StatsMod &stats) {
  _type = STATS;
  _stats = stats;
}

bool BuffType::doesntStackWith(const BuffType &otherType) const {
  if (_nonStackingCategory.empty()) return false;
  return _nonStackingCategory == otherType._nonStackingCategory;
}

Buff::Buff(const BuffType &type, Entity &owner, Entity &caster)
    : _type(&type),
      _owner(&owner),
      _caster(&caster),
      _timeRemaining(type.duration()) {}

Buff::Buff(const BuffType &type, Entity &owner, ms_t timeRemaining)
    : _type(&type), _owner(&owner), _timeRemaining(timeRemaining) {}

bool Buff::doesntStackWith(const BuffType &otherType) const {
  return _type->doesntStackWith(otherType);
}

void Buff::clearCasterIfEqualTo(const Entity &casterToRemove) const {
  if (_caster == &casterToRemove) _caster = nullptr;
}

void Buff::update(ms_t timeElapsed) {
  if (_expired) {
    SERVER_ERROR("Trying to update expired buff");
    return;
  }

  // Server::debug()(toString(_timeRemaining));

  auto ticks = _type->tickTime() > 0;
  if (ticks) {
    _timeSinceLastProc += timeElapsed;
    auto shouldProc = _timeSinceLastProc > _type->tickTime();
    if (shouldProc) {
      // A null caster circumvents deleted-memory errors, but for now makes any
      // on-tick actions ineffectual.  At some point casters should become
      // references again, allowing debuff effects from offline/dead entities.
      if (_caster) proc();
      _timeSinceLastProc -= _type->tickTime();
    }
  }

  auto expires = _timeRemaining > 0;
  if (!expires) return;
  if (timeElapsed >= _timeRemaining) {
    _expired = true;
    return;
  }
  _timeRemaining -= timeElapsed;
}

void Buff::manuallyChangeTimeRemaining(ms_t newTimeRemaining) {
  _timeRemaining = newTimeRemaining;
}

void Buff::proc(Entity *target) const {
  if (target == nullptr) target = _owner;
  _type->effect().execute(*_caster, *target);
}
