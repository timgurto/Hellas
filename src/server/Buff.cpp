#include "Buff.h"

void BuffType::stats(const StatsMod & stats) {
    _type = STATS;
    _stats = stats;
}

SpellEffect & BuffType::effect() {
    _type = SPELL_OVER_TIME;
    return _effect;
}

Buff::Buff(const BuffType & type, Entity & owner, Entity & caster):
_type(type),
_owner(owner),
_caster(caster)
{}

void Buff::update(ms_t timeElapsed) const {
    if (_type.tickTime() == 0)
        return;

    _timeSinceLastProc += timeElapsed;
    auto shouldProc = _timeSinceLastProc > _type.tickTime();
    if (shouldProc) {
        _type.effect().execute(_caster, _owner);
        _timeSinceLastProc -= _type.tickTime();
    }
}
