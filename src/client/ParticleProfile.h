#ifndef PARTICLE_PROFILE_H
#define PARTICLE_PROFILE_H

#include <string>

#include "SpriteType.h"
#include "../NormalVariable.h"

class Particle;

class ParticleProfile{
    std::string _id;
    double _particlesPerSecond; // For gathering, crafting
    double _gravity;
    bool _noZDimension; // Do not use a virtual z dimension.
    NormalVariable
        _particlesPerHit, // For attacking
        _distance,
        _altitude,
        _velocity,
        _fallSpeed,
        _lifespan; // The particle will disappear after this time
    Point _direction;
    Uint8 _alpha;
    std::vector<const SpriteType *> _varieties, _pool;

public:
    static const double DEFAULT_GRAVITY; // px/s/s
    static const ms_t DEFAULT_LIFESPAN_MEAN, DEFAULT_LIFESPAN_SD;

    ParticleProfile(const std::string &id);
    ~ParticleProfile();
    
    void particlesPerSecond(double f) { _particlesPerSecond = f; }
    void particlesPerHit(double mean, double sd) { _particlesPerHit = NormalVariable(mean, sd); }
    void distance(double mean, double sd) { _distance = NormalVariable(mean, sd); }
    void altitude(double mean, double sd) { _altitude = NormalVariable(mean, sd); }
    void velocity(double mean, double sd) { _velocity = NormalVariable(mean, sd); }
    void direction(const Point &p) { _direction = p; }
    void fallSpeed(double mean, double sd) { _fallSpeed = NormalVariable(mean, sd); }
    void lifespan(double mean, double sd) { _lifespan = NormalVariable(mean, sd); }
    void gravityuModifer(double mod) { _gravity *= mod; }
    void noZDimension() { _noZDimension = true; }
    void alpha(Uint8 n) { _alpha = n; }

    struct ptrCompare{
        bool operator()(const ParticleProfile *lhs, const ParticleProfile *rhs){
            return lhs->_id < rhs->_id;
        }
    };

    /*
    Register a new SpriteType with the client describing a variety of particle.  Also, save it to
    _varieties so that it might be chosen when a new Particle is created.
    */
    void addVariety(const std::string &imageFile, size_t count = 1);

    /*
    Create a new particle and return its pointer.  The caller takes responsibility for freeing it.
    */
    Particle *instantiate(const Point &location) const;

    /*
    Conceptually, returns delta * _particlesPerSecond.  However, since we want a whole number, and
    since an accurate return value will often be < 0, randomization is used to give the correct
    results over time, with the bonus of looking good.
    */
    size_t numParticlesContinuous(double delta) const;

    // Generate a random number of particles for a single hit, e.g. on an attack.
    size_t numParticlesDiscrete() const;
};

#endif
