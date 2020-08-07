#include "HasSounds.h"

#include "Client.h"

void HasSounds::setSoundProfile(const Client &client,
                                const std::string &profileID) {
  _profile = client.findSoundProfile(profileID);
}

void HasSounds::playSoundOnce(const Client &client,
                              const SoundType &type) const {
  if (!_profile) return;
  _profile->playOnce(client, type);
}

ms_t HasSounds::soundPeriod() const {
  if (!_profile) return DEFAULT_PERIOD;
  return _profile->period();
}
