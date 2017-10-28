#ifndef STATS_H
#define STATS_H

#include "types.h"

struct StatsMod;

// Describes base-level player stats.
struct Stats{
    Hitpoints
        health,
        attack;

    ms_t
        attackTime;

    double
        speed;

    Stats();
    
    Stats &operator+=(const Stats &rhs);
    const Stats operator+(const Stats &rhs) const;

    const Stats &operator&=(const StatsMod &rhs);
    Stats operator&(const StatsMod &mod) const;
};

// Describes modifiers for player stats, e.g. for gear.
struct StatsMod{
    // Additive
    int
        health,
        attack;

    // Multiplicative
    double
        attackTime,
        speed;

    StatsMod();
};

#endif
