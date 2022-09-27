#ifndef STATS_H
#define STATS_H

#include <map>
#include <set>
#include <vector>

#include "SpellSchool.h"
#include "combatTypes.h"
#include "types.h"

// Describes modifiers for player stats, e.g. for gear.
struct StatsMod {
  std::vector<std::string> toStrings() const;
  std::string buffDescription() const;  // e.g. "Grant 1 health to ", "Stun "

  // Additive
  int maxHealth{0}, maxEnergy{0}, followerLimit{0};
  Regen hps{0}, eps{0};
  Hundredths blockValue{0};
  BasisPoints magicDamage{0}, physicalDamage{0}, healing{0};
  ArmourClass armor{0}, airResist{0}, earthResist{0}, fireResist{0},
      waterResist{0};
  BasisPoints hit{0}, crit{0}, critResist{0}, dodge{0}, block{0};
  Percentage gatherBonus{0};
  BasisPoints unlockBonus{0};

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

  std::set<std::string> namesOfIncludedStats() const;
};

struct CompositeStat {
  std::string name;
  std::string description;
  StatsMod stats;
};

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
  BasisPoints unlockBonus{0};

  std::map<std::string, int> composites;
  const int &getComposite(std::string statName) const;

  static std::map<std::string, CompositeStat> compositeDefinitions;
};

#endif
