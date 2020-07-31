#pragma once

#include "SoundProfile.h"

class Client;

class HasSounds {
 public:
  void setSoundProfile(const std::string &profileID);

  void playSoundOnce(const SoundType &type) const;
  ms_t soundPeriod() const;

 private:
  const SoundProfile *_profile{nullptr};

  static const ms_t DEFAULT_PERIOD = 1000;
};
