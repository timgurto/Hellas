#ifndef STATS_H
#define STATS_H

#include "types.h"

struct StatsMod;

// Describes base-level player stats.
struct Stats{
    Hitpoints
        health = 0,
        attack = 0,
        blockValue = 0;

    Energy
        energy = 0;

    Regen
        hps = 0,
        eps = 0;

    Percentage
        armor = 0,
        hit = 0,
        crit = 0,
        critResist = 0,
        dodge = 0,
        block = 0,
        airResist = 0,
        earthResist = 0,
        fireResist = 0,
        waterResist = 0;

    BonusDamage
        magicDamage = 0,
        physicalDamage = 0,
        healing = 0;

    ms_t
        attackTime = 0;

    double
        speed = 0;
    
    Stats &operator+=(const Stats &rhs);
    const Stats operator+(const Stats &rhs) const;

    const Stats &operator&=(const StatsMod &rhs);
    Stats operator&(const StatsMod &mod) const;
};

// Describes modifiers for player stats, e.g. for gear.
struct StatsMod{
    // Additive
    int
        armor = 0,
        health = 0,
        hps = 0,
        energy = 0,
        eps = 0,
        attack = 0,
        blockValue = 0;
    BonusDamage
        magicDamage = 0,
        physicalDamage = 0,
        healing = 0;
    Percentage
        hit = 0,
        crit = 0,
        critResist = 0,
        dodge = 0,
        block = 0,
        airResist = 0,
        earthResist = 0,
        fireResist = 0,
        waterResist = 0;

    // Multiplicative
    double
        attackTime = 1.0,
        speed = 1.0;
};

#endif
