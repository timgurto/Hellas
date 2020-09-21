#ifndef STATS_H
#define STATS_H

#include <map>
#include <vector>

#include "SpellSchool.h"
#include "combatTypes.h"
#include "types.h"

struct StatsMod;

// Describes base-level player stats.
struct Stats {
  Hitpoints maxHealth{0};

  Energy maxEnergy{0};

  Regen hps{0}, eps{0};

  ArmourClass armor{0}, airResist{0}, earthResist{0}, fireResist{0},
      waterResist{0};
  BasisPoints hit{0}, crit{0}, critResist{0}, dodge{0}, block{0};
  Percentage gatherBonus{0};

  Hitpoints weaponDamage{0};
  BasisPoints magicDamage{0}, physicalDamage{0}, healing{0};
  SpellSchool weaponSchool{SpellSchool::PHYSICAL};

  Hundredths blockValue{0};

  ms_t attackTime{0};

  double speed{0};

  bool stunned{false};

  int followerLimit{0};

  const Stats &operator&=(StatsMod rhs);
  Stats operator&(const StatsMod &mod) const;
  void modify(const StatsMod &mod);

  ArmourClass resistanceByType(SpellSchool school) const;
  BasisPoints bonusUnlockChance{0};

  static std::map<std::string, StatsMod> compositeDefinitions;
};

// Describes modifiers for player stats, e.g. for gear.
struct StatsMod {
  std::vector<std::string> toStrings() const;
  std::string buffDescription() const;  // e.g. "Grant 1 health to ", "Stun "

  // Additive
  int maxHealth{0}, hps{0}, maxEnergy{0}, eps{0}, followerLimit{0};
  Hundredths blockValue{0};
  BasisPoints magicDamage{0}, physicalDamage{0}, healing{0};
  ArmourClass armor{0}, airResist{0}, earthResist{0}, fireResist{0},
      waterResist{0};
  BasisPoints hit{0}, crit{0}, critResist{0}, dodge{0}, block{0};
  Percentage gatherBonus{0};

  // Replacement
  ms_t attackTime{0};
  Hitpoints weaponDamage{0};
  SpellSchool weaponSchool{SpellSchool::PHYSICAL};

  // Multiplicative
  double speed{1.0};

  // Flag
  bool stuns{false};

  // Composite stats
  std::map<std::string, int> composites;

  StatsMod operator*(int scalar) const;
};

#endif
