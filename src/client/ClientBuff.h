#pragma once

#include "../Stats.h"
#include "ClientSpell.h"
#include "Texture.h"

class ClientBuffType {
 public:
  using ID = std::string;
  using Name = std::string;
  using Description = std::string;

  ClientBuffType() {}
  ClientBuffType(const std::string &iconFile);
  void name(const Name &name) { _name = name; }
  const Name &name() const { return _name; }
  void id(const ID &id) { _id = id; }
  const ID &id() const { return _id; }
  void description(const Description &description) {
    _description = description;
  }
  const Description &description() const { return _description; }
  bool hasDescription() const { return !_description.empty(); }
  const Texture &icon() const { return _icon; }
  void effectName(const std::string &name) { _effectName = name; }
  const std::string &effectName() const { return _effectName; }
  void effectArgs(const ClientSpell::Args &args) { _effectArgs = args; }
  const ClientSpell::Args &effectArgs() const { return _effectArgs; }
  void stats(const StatsMod &s) { _stats = s; }
  const StatsMod &stats() const { return _stats; }
  void duration(int s) { _duration = s; }
  const int duration() const { return _duration; }
  void tickTime(ms_t ms) { _tickTime = ms; }
  const ms_t tickTime() const { return _tickTime; }
  void hasHitEffect() { _hasHitEffect = true; }
  bool hasHitEffect() const { return _hasHitEffect; }
  void cancelOnOOE() { _cancelsOnOOE = true; }
  bool cancelsOnOOE() const { return _cancelsOnOOE; }
  void particles(const std::string &profileName) { _particles = profileName; }
  const std::string &particles() const { return _particles; }
  void effect(const Texture &image, const ScreenPoint &offset);
  bool hasEffect() const { return {_effect}; }
  const Texture &effectImage() const { return _effect; }
  const ScreenPoint &effectOffset() const { return _effectOffset; }
  void makesTargetInvisible() { _makesTargetInvisible = true; }
  bool isTargetInvisible() const { return _makesTargetInvisible; }

 private:
  Texture _icon;
  ID _id{};
  Name _name{};
  Description _description{};
  Texture _effect;
  bool _makesTargetInvisible{false};
  ScreenPoint _effectOffset{};

  int _duration{0};
  bool _hasHitEffect{false};
  ms_t _tickTime{0};
  bool _cancelsOnOOE{false};
  std::string _effectName{};
  ClientSpell::Args _effectArgs{};
  StatsMod _stats{};
  std::string _particles = "";
};

using ClientBuffTypes = std::map<ClientBuffType::ID, ClientBuffType>;
