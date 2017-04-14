#include "Client.h"
#include "SoundProfile.h"

SoundsRecord SoundProfile::loopingSounds;

SoundProfile::SoundProfile(const std::string &id):
_id(id)
{}

bool SoundProfile::operator<(const SoundProfile &rhs) const{
    return _id < rhs._id;
}

void SoundProfile::set(const SoundType &type, std::string &filename){
    _sounds[type] = Mix_LoadWAV(("Sounds/" + filename + ".ogg").c_str());
}

void SoundProfile::playOnce(const SoundType &type) const{
    checkAndPlaySound(type, false);
}

void SoundProfile::startLooping(const SoundType &type, const void *source) const{
    Channel channel = checkAndPlaySound(type, true);
    loopingSounds.set(type, source, channel);
}

Channel SoundProfile::checkAndPlaySound(const SoundType &type, bool loop) const{
    Mix_Chunk *sound = _sounds.at(type);
    if (sound == nullptr){
        Client::_instance->debug() << Color::WARNING << "\"" << type << "\" sound not found."
                                   << Log::endl;
        return NO_CHANNEL;
    }
    int loopArg = loop ? -1 : 0;
    return Mix_PlayChannel(-1, sound, loopArg);
}

void SoundProfile::stopLooping(const SoundType &type, const void *source) const{
    Channel channel = loopingSounds.getChannel(type, source);
    Mix_HaltChannel(channel);
    loopingSounds.unset(channel);
}



const SoundsRecord::Entry SoundsRecord::Entry::BLANK("", nullptr);

SoundsRecord::SoundsRecord():
_record(Client::MIXING_CHANNELS, Entry::BLANK)
{}

void SoundsRecord::set(const SoundType &type, const void *source, Channel channel){
    if (channel == NO_CHANNEL)
        return;
    _record[channel] = Entry(type, source);
}

Channel SoundsRecord::getChannel(const SoundType &type, const void *source) const{
    for (Channel channel = 0; channel != Client::MIXING_CHANNELS; ++channel){
        const Entry &entry = _record[channel];
        if (entry.source == source && entry.type == type){
            return channel;
        }
    }
    return NO_CHANNEL;
}

void SoundsRecord::unset(Channel channel){
    if (channel == NO_CHANNEL)
        return;
    _record[channel] = Entry::BLANK;
}
