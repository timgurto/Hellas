#pragma once

#include <map>
#include <string>

#include "../Message.h"
#include "../Optional.h"
#include "../Podes.h"
#include "../SpellSchool.h"
#include "Projectile.h"

class ParticleProfile;
class SoundProfile;

class ClientSpell {
 public:
  enum TargetType {
    SELF,
    FRIENDLY,
    ENEMY,
    ALL,

    NUM_TARGET_TYPES
  };

  struct Args {
    int i1{0};
    double d1{0};
    std::string s1{};
  };

  ClientSpell(const std::string &id, Client &client);

  const std::string &id() const { return _id; }
  void projectile(const Projectile::Type *p) { _projectile = p; }
  const Projectile::Type *projectile() const { return _projectile; }
  void sounds(const SoundProfile *p) { _sounds = p; }
  const SoundProfile *sounds() const { return _sounds; }
  void impactParticles(const ParticleProfile *p) { _impactParticles = p; }
  const ParticleProfile *impactParticles() const { return _impactParticles; }
  const Message &castMessage() const { return _castMessage; }
  void icon(const Texture &iconTexture);
  const Texture &icon() const { return _icon; }
  const Tooltip &tooltip() const;
  void name(const std::string &s) { _name = s; }
  const std::string &name() const { return _name; }
  void cost(Energy c) { _cost = c; }
  void effectName(const std::string effect) { _effectName = effect; }
  void effectArgs(const Args &args) { _effectArgs = args; }
  void range(Podes r) { _range = r; }
  void radius(Podes r) {
    _range = r;
    _isAoE = true;
  }
  void school(SpellSchool school) { _school = school; }
  void targetType(TargetType t) { _targetType = t; }
  void cooldown(int s) { _cooldown = s; }
  int cooldown() const { return _cooldown; }
  void customDescription(const std::string &description) {
    _customDescription = description;
  }
  Client &client() { return _client; }

  std::string createEffectDescription() const;

 private:
  const Projectile::Type *_projectile = nullptr;
  const SoundProfile *_sounds = nullptr;
  const ParticleProfile *_impactParticles = nullptr;
  Message _castMessage{};
  Texture _icon;
  mutable Optional<Tooltip> _tooltip;
  std::string _id = {};
  std::string _name = {};
  Energy _cost = 0;
  Podes _range = 0;
  bool _isAoE = false;
  SpellSchool _school;
  std::string _customDescription{};  // Used if not empty

  std::string _effectName = {};
  Args _effectArgs = {};
  TargetType _targetType = NUM_TARGET_TYPES;
  int _cooldown{0};  // in seconds

  Client &_client;
};

using ClientSpells = std::map<std::string, ClientSpell *>;
