#ifndef SOUND_PROFILE_H
#define SOUND_PROFILE_H

#include <string>

#include <SDL_mixer.h>

class SoundProfile{
    std::string _id;

    Mix_Chunk
        *_defend,
        *_death;

public:
    SoundProfile(const std::string &id);

    bool operator<(const SoundProfile &rhs) const;

    void setDefend(const std::string &filename);
    void setDeath(const std::string &filename);
    
    void playDefend() const;
    void playDeath() const;
};

#endif
