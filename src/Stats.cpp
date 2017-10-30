#include <cassert>

#include "Stats.h"
#include "util.h"

Stats &Stats::operator+=(const Stats &rhs){
    health += rhs.health;
    energy += rhs.energy;
    hps += rhs.hps;
    eps += rhs.eps;
    hit += rhs.hit;
    crit += rhs.crit;
    attack += rhs.attack;
    attackTime += rhs.attackTime;
    speed += rhs.speed;

    return *this;
}

const Stats Stats::operator+(const Stats &rhs) const{
    Stats ret = *this;
    ret += rhs;
    return ret;
}

const Stats &Stats::operator&=(const StatsMod &mod){
    if (mod.health < 0 && -mod.health > static_cast<int>(health))
        health = 0;
    else
        health += mod.health;

    if (mod.energy < 0 && -mod.energy > static_cast<int>(energy))
        energy = 0;
    else
        energy += mod.energy;

    if (mod.hps < 0 && -mod.hps > static_cast<int>(hps))
        hps = 0;
    else
        hps += mod.hps;

    if (mod.eps < 0 && -mod.eps > static_cast<int>(eps))
        eps = 0;
    else
        eps += mod.eps;

    if (mod.hit < 0 && -mod.hit > static_cast<int>(hit))
        hit = 0;
    else
        hit += mod.hit;

    if (mod.crit < 0 && -mod.crit > static_cast<int>(crit))
        crit = 0;
    else
        crit += mod.crit;

    if (mod.attack < 0 && -mod.attack > static_cast<int>(attack))
        attack = 0;
    else
        attack += mod.attack;

    assert(mod.attackTime >= 0);
    if (mod.attackTime != 1.0)
        attackTime = toInt(attackTime * mod.attackTime);
    
    assert(mod.speed >= 0);
    if (mod.speed != 1.0)
        speed *= mod.speed;

    return *this;
}

Stats Stats::operator&(const StatsMod &mod) const{
    Stats stats = *this;
    stats &= mod;
    return stats;
}
