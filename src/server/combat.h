#pragma once

enum CombatType {
    DAMAGE,
    HEAL, // Can crit
    DEBUFF // Can't crit
};

enum CombatResult {
    FAIL,

    HIT,
    CRIT,
    BLOCK,
    DODGE,
    MISS
};
