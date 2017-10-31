#pragma once

#include <map>
#include <string>

#include "Projectile.h"
#include "../Podes.h"

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
    const Texture &tooltip() const;
    void name(const std::string &s) { _name = s; }
    void cost(Energy c) { _cost = c; }
    void effectName(const std::string effect) { _effectName = effect; }
    void addEffectArg(int arg) { _effectArgs.push_back(arg); }
    void range(Podes r) { _range = r; }
    void radius(Podes r) { _range = r;  _isAoE = true; }

private:
    const Projectile::Type *_projectile = nullptr;
    const SoundProfile *_sounds = nullptr;
    const ParticleProfile *_impactParticles = nullptr;
    std::string _castMessage;
    Texture _icon;
    mutable Texture _tooltip = {};
    std::string _name = {};
    Energy _cost = 0;
    Podes _range = 0;
    bool _isAoE = false;

    std::string _effectName = {};
    std::vector<int> _effectArgs = {};

    std::string createEffectDescription() const;
};

using ClientSpells = std::map<std::string, ClientSpell *>;
