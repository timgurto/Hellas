#pragma once

#include "../Stats.h"
#include "ClientSpell.h"
#include "Texture.h"

class ClientBuffType {
 public:
  using ID = std::string;
  using Name = std::string;

  ClientBuffType() {}
  ClientBuffType(const std::string &iconFile);
  void name(const Name &name) { _name = name; }
  const Name &name() const { return _name; }
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

 private:
  Texture _icon;
  std::string _name{};

  int _duration{0};
  bool _hasHitEffect{false};
  ms_t _tickTime{0};
  bool _cancelsOnOOE{false};
  std::string _effectName{};
  ClientSpell::Args _effectArgs{};
  StatsMod _stats{};
};

using ClientBuffTypes = std::map<ClientBuffType::ID, ClientBuffType>;
