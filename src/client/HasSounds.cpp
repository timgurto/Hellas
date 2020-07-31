#include "HasSounds.h"

#include "Client.h"

void HasSounds::setSoundProfile(const std::string &profileID) {
  _profile = Client::findSoundProfile(profileID);
}

void HasSounds::playSoundOnce(const SoundType &type) const {
  if (!_profile) return;
  _profile->playOnce(type);
}

ms_t HasSounds::soundPeriod() const {
  if (!_profile) return DEFAULT_PERIOD;
  return _profile->period();
}
