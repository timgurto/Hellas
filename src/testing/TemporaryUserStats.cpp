#include "TemporaryUserStats.h"

TemporaryUserStats::TemporaryUserStats() {
  storeCurrentSettings();
  _new = _old;
}

TemporaryUserStats::~TemporaryUserStats() { restoreOriginalSettings(); }

TemporaryUserStats& TemporaryUserStats::hps(Regen v) {
  _new.hps = v;
  apply();
  return *this;
}

void TemporaryUserStats::apply() { User::OBJECT_TYPE.baseStats(_new); }

void TemporaryUserStats::storeCurrentSettings() {
  _old = User::OBJECT_TYPE.baseStats();
}

void TemporaryUserStats::restoreOriginalSettings() {
  User::OBJECT_TYPE.baseStats(_old);
}
