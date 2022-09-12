#include "SoundProfile.h"

#include <SDL_mixer.h>

#include "Client.h"
#include "Options.h"

extern Args cmdLineArgs;
extern Options options;

SoundsRecord SoundProfile::loopingSounds;

SoundProfile::SoundProfile(const std::string &id) : _id(id), _period(0) {}

bool SoundProfile::operator<(const SoundProfile &rhs) const {
  return _id < rhs._id;
}

void SoundProfile::add(const SoundType &type, std::string &filename) {
  Mix_Chunk *sound = Mix_LoadWAV(("Sounds/" + filename + ".ogg").c_str());
  _sounds[type].add(sound);
}

void SoundProfile::playOnce(const Client &client, const SoundType &type) const {
  if (!this) {
    client.showErrorMessage("Sound profile does not exist."s,
                            Color::CHAT_ERROR);
    return;
  }
  checkAndPlaySound(client, type, false);
}

void SoundProfile::startLooping(const Client &client, const SoundType &type,
                                const void *source) const {
  Channel channel = checkAndPlaySound(client, type, true);
  loopingSounds.set(type, source, channel);
}

Channel SoundProfile::checkAndPlaySound(const Client &client,
                                        const SoundType &type,
                                        bool loop) const {
  const auto gameIsMuted =
      cmdLineArgs.contains("mute") || !options.audio.enableSFX;
  if (gameIsMuted) return NO_CHANNEL;

  auto it = _sounds.find(type);
  if (it == _sounds.end()) {
#ifdef _DEBUG
    client.showErrorMessage("\""s + _id + ":" + type + "\" sound not found."s,
                            Color::CHAT_ERROR);
#endif
    return NO_CHANNEL;
  }
  SoundVariants variants = it->second;
  Mix_Chunk *sound = variants.choose();
  if (sound == nullptr) {
    client.showErrorMessage(
        "\""s + _id + ":" + type + "\" sound variant not found."s,
        Color::CHAT_ERROR);
    return NO_CHANNEL;
  }
  int loopArg = loop ? -1 : 0;
  return Mix_PlayChannel(-1, sound, loopArg);
}

void SoundProfile::stopLooping(const SoundType &type,
                               const void *source) const {
  Channel channel = loopingSounds.getChannel(type, source);
  Mix_HaltChannel(channel);
  loopingSounds.unset(channel);
}

Mix_Chunk *SoundVariants::choose() const {
  size_t numChoices = _variants.size();
  if (numChoices == 0) return nullptr;
  size_t choiceIndex = rand() % numChoices;
  return _variants[choiceIndex];
}

const SoundsRecord::Entry SoundsRecord::Entry::BLANK("", nullptr);

SoundsRecord::SoundsRecord() : _record(Client::MIXING_CHANNELS, Entry::BLANK) {}

void SoundsRecord::set(const SoundType &type, const void *source,
                       Channel channel) {
  if (channel == NO_CHANNEL) return;
  _record[channel] = Entry(type, source);
}

Channel SoundsRecord::getChannel(const SoundType &type,
                                 const void *source) const {
  for (Channel channel = 0; channel != Client::MIXING_CHANNELS; ++channel) {
    const Entry &entry = _record[channel];
    if (entry.source == source && entry.type == type) {
      return channel;
    }
  }
  return NO_CHANNEL;
}

void SoundsRecord::unset(Channel channel) {
  if (channel == NO_CHANNEL) return;
  _record[channel] = Entry::BLANK;
}
