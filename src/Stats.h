#ifndef STATS_H
#define STATS_H

// Describes base-level player stats.
struct Stats{
    health_t
        health,
        attack;

    ms_t
        attackTime;

    double
        speed;

    Stats():
        health(0),
        attack(0),
        attackTime(0),
        speed(0)
    {}
};

#endif
