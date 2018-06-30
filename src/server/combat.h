#pragma once

enum CombatType {
  DAMAGE,
  HEAL,        // Can crit
  DEBUFF,      // Can't crit
  THREAT_MOD,  // Can't block/dodge
};

enum CombatResult {
  FAIL,

  HIT,
  CRIT,
  BLOCK,
  DODGE,
  MISS
};
