#include "SoundProfile.h"

SoundProfile::SoundProfile(const std::string &id):
_id(id),
_defend(nullptr),
_death(nullptr)
{}

bool SoundProfile::operator<(const SoundProfile &rhs) const{
    return _id < rhs._id;
}


void SoundProfile::setDefend(const std::string &filename){
    _defend = Mix_LoadWAV(("Sounds/" + filename + ".wav").c_str());
}

void SoundProfile::setDeath(const std::string &filename){
    _death = Mix_LoadWAV(("Sounds/" + filename + ".wav").c_str());
}

void SoundProfile::playDefend() const{
    if (_defend != nullptr) {
        Mix_PlayChannel(-1, _defend, 0);
    }
}

void SoundProfile::playDeath() const{
    if (_death != nullptr) {
        Mix_PlayChannel(-1, _death, 0);
    }
}
