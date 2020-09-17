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

TemporaryUserStats& TemporaryUserStats::hit(BasisPoints v) {
  _new.hit = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::crit(BasisPoints v) {
  _new.crit = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::critResist(BasisPoints v) {
  _new.critResist = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::dodge(BasisPoints v) {
  _new.dodge = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::block(BasisPoints v) {
  _new.block = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::armour(ArmourClass v) {
  _new.armor = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::weaponDamage(Hitpoints v) {
  _new.weaponDamage = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::gatherBonus(Percentage v) {
  _new.gatherBonus = v;
  apply();
  return *this;
}

TemporaryUserStats& TemporaryUserStats::followerLimit(int v) {
  _new.followerLimit = v;
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