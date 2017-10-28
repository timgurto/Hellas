#pragma once

#include <map>
#include <string>

#include "Projectile.h"

class ClientSpell {
public:
    ClientSpell(const std::string &id);

    void projectile(const Projectile::Type *p) { _projectile = p; }
    const Projectile::Type *projectile() const { return _projectile; }
    void sounds(const SoundProfile *p) { _sounds = p; }
    const SoundProfile *sounds() const { return _sounds; }
    void impactParticles(const ParticleProfile *p) { _impactParticles = p; }
    const ParticleProfile *impactParticles() const { return _impactParticles; }
    const std::string &castMessage() const { return _castMessage; }
    const Texture &icon() const { return _icon; }

private:
    const Projectile::Type *_projectile = nullptr;
    const SoundProfile *_sounds = nullptr;
    const ParticleProfile *_impactParticles = nullptr;
    std::string _castMessage;
    Texture _icon;
};

using ClientSpells = std::map<std::string, ClientSpell *>;
