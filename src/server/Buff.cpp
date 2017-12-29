#include <cassert>

#include "Buff.h"
#include "Server.h"

void BuffType::stats(const StatsMod & stats) {
    _type = STATS;
    _stats = stats;
}

SpellEffect & BuffType::effect() {
    _type = SPELL_OVER_TIME;
    return _effect;
}

Buff::Buff(const BuffType & type, Entity & owner, Entity & caster):
_type(&type),
_owner(&owner),
_caster(&caster),
_timeRemaining(type.duration())
{}

void Buff::clearCasterIfEqualTo(const Entity & casterToRemove) const {
    if (_caster == &casterToRemove)
        _caster = nullptr;
}

void Buff::update(ms_t timeElapsed) {
    assert(!_expired);

    Server::debug()(toString(_timeRemaining));

    auto ticks = _type->tickTime() > 0;
    if (ticks) {
        _timeSinceLastProc += timeElapsed;
        auto shouldProc = _timeSinceLastProc > _type->tickTime();
        if (shouldProc) {
            // A null caster circumvents deleted-memory errors, but for now makes any on-tick
            // actions ineffectual.  At some point casters should become references again, allowing
            // debuff effects from offline/dead entities.
            if (_caster)
                _type->effect().execute(*_caster, *_owner);
            _timeSinceLastProc -= _type->tickTime();
        }
    }

    auto expires = _timeRemaining > 0;
    if (!expires)
        return;
    if (timeElapsed >= _timeRemaining) {
        _expired = true;
        return;
    }
    _timeRemaining -= timeElapsed;
}
