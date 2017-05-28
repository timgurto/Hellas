#include <cassert>

#include "Stats.h"
#include "util.h"

Stats::Stats():
    health(0),
    attack(0),
    attackTime(0),
    speed(0.0)
{}

StatsMod::StatsMod():
    health(0),
    attack(0),

    attackTime(1.0),
    speed(1.0)
{}

Stats &Stats::operator+=(const Stats &rhs){
    health += rhs.health;
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
