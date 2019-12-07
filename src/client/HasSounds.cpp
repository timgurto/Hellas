#include "HasSounds.h"

#include "Client.h"

void HasSounds::setSoundProfile(const std::string &profileID) {
  const Client &client = Client::instance();
  _profile = client.findSoundProfile(profileID);
}

void HasSounds::playSoundOnce(const SoundType &type) const {
  if (!_profile) return;
  _profile->playOnce(type);
}

ms_t HasSounds::soundPeriod() const {
  if (!_profile) return DEFAULT_PERIOD;
  return _profile->period();
}
