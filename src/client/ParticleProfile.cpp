#include <cassert>

#include "Particle.h"
#include "ParticleProfile.h"

const double ParticleProfile::DEFAULT_GRAVITY = 100.0;
const ms_t ParticleProfile::DEFAULT_LIFESPAN_MEAN = 60000;
const ms_t ParticleProfile::DEFAULT_LIFESPAN_SD = 10000;

ParticleProfile::ParticleProfile(const std::string &id):
_id(id),
_particlesPerSecond(0),
_gravity(DEFAULT_GRAVITY),
_lifespan(DEFAULT_LIFESPAN_MEAN, DEFAULT_LIFESPAN_SD),
_noZDimension(false)
{}

ParticleProfile::~ParticleProfile(){
    for (const SpriteType *variety : _varieties)
        delete variety;
}

void ParticleProfile::addVariety(const std::string &imageFile, const Rect &drawRect, size_t count){
    SpriteType *particleType = new SpriteType(drawRect, "Images/Particles/" + imageFile + ".png");
    _varieties.push_back(particleType); // _varieties owns the pointers.
    for (size_t i = 0; i != count; ++i)
        _pool.push_back(particleType);
}

Particle *ParticleProfile::instantiate(const Point &location) const{
    // Choose random variety
    assert (!_pool.empty());
    size_t index = rand() % _pool.size();
    const SpriteType &variety = *_pool[index];

    // Choose random position, then set location
    double angle = 2 * PI * randDouble();
    double distance = _distance.generate();
    Point locationOffset(cos(angle) * distance, sin(angle) * distance);
    Point startingLoc = location + locationOffset;

    // Choose random direction, then set velocity
    angle = 2 * PI * randDouble();
    double velocity = _velocity.generate();
    Point startingVelocity(cos(angle) * velocity, sin(angle) * velocity);

    if (_noZDimension){
        startingLoc.y = location.y;
        startingVelocity.y = 0;
    }


    return new Particle(startingLoc, variety.image(), variety.drawRect(), startingVelocity,
                        _altitude.generate(), _fallSpeed.generate(), _gravity,
                        max(0, toInt(_lifespan.generate())));
}

size_t ParticleProfile::numParticlesContinuous(double delta) const{
    size_t total = 0;
    double particlesToAdd = _particlesPerSecond * delta;
    if (particlesToAdd >= 1){ // Whole part
        total = static_cast<size_t>(particlesToAdd);
        particlesToAdd -= total;
    }
    if (randDouble() <= particlesToAdd)
        ++total;
    return total;
}

size_t ParticleProfile::numParticlesDiscrete() const{
    return max(0, toInt(_particlesPerHit.generate()));
}
