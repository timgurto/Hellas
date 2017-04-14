#include "Client.h"
#include "SoundProfile.h"

const SoundProfile::Channel SoundProfile::NO_CHANNEL = -1;
std::vector<std::pair<std::string, const void *> >
        SoundProfile::repeatingSounds(static_cast<size_t>(Client::MIXING_CHANNELS));

SoundProfile::SoundProfile(const std::string &id):
_id(id)
{}

bool SoundProfile::operator<(const SoundProfile &rhs) const{
    return _id < rhs._id;
}

void SoundProfile::set(const std::string &type, std::string &filename){
    _sounds[type] = Mix_LoadWAV(("Sounds/" + filename + ".ogg").c_str());
    auto error = Mix_GetError();
}

void SoundProfile::play(const std::string &type) const{
    Mix_Chunk *sound = _sounds.at(type);
    if (sound == nullptr){
        Client::_instance->debug() << Color::WARNING << "\"" << type << "\" sound not found."
                                   << Log::endl;
        return;
    }
    Mix_PlayChannel(-1, sound, 0);
}

void SoundProfile::playRepeated(const std::string &type, const void *source) const{
    Mix_Chunk *sound = _sounds.at(type);
    if (sound == nullptr){
        Client::_instance->debug() << Color::WARNING << "\"" << type << "\" sound not found."
                                   << Log::endl;
        return;
    }
    Channel channel = Mix_PlayChannel(-1, sound, -1);
    repeatingSounds[channel] = std::make_pair(type, source);
}

void SoundProfile::stopRepeated(const std::string &type, const void *source) const{
    for (size_t i = 0; i != Client::MIXING_CHANNELS; ++i){
        auto &pair = repeatingSounds[i];
        if (pair.first == type && pair.second == source){
            Mix_HaltChannel(i);
            pair = std::make_pair("", nullptr);
            break;
        }
    }
}
