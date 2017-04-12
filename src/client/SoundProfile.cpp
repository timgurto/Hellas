#include "Client.h"
#include "SoundProfile.h"

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
    Mix_Chunk *sound = (_sounds.at(type));
    if (sound == nullptr){
        Client::_instance->debug() << Color::WARNING << "\"" << type << "\" sound not found."
                                   << Log::endl;
        return;
    }
    Mix_PlayChannel(-1, sound, 0);
}
