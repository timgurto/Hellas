#pragma once

#include <map>
#include <string>

#include "Projectile.h"
#include "../Podes.h"
#include "../SpellSchool.h"

class ParticleProfile;
class SoundProfile;

class ClientSpell {
public:
    struct Args {
        int i1 = 0;
        std::string s1{};
    };

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
    const std::string &name() const { return _name; }
    void cost(Energy c) { _cost = c; }
    void effectName(const std::string effect) { _effectName = effect; }
    void effectArgs(const Args &args) { _effectArgs = args; }
    void range(Podes r) { _range = r; }
    void radius(Podes r) { _range = r;  _isAoE = true; }
    void school(SpellSchool school) { _school = school; }

private:
    const Projectile::Type *_projectile = nullptr;
    const SoundProfile *_sounds = nullptr;
    const ParticleProfile *_impactParticles = nullptr;
    std::string _castMessage{};
    Texture _icon;
    mutable Texture _tooltip = {};
    std::string _id = {};
    std::string _name = {};
    Energy _cost = 0;
    Podes _range = 0;
    bool _isAoE = false;
    SpellSchool _school;

    std::string _effectName = {};
    Args _effectArgs = {};

    std::string createEffectDescription() const;
};

using ClientSpells = std::map<std::string, ClientSpell *>;
