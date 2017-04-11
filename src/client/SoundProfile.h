#ifndef SOUND_PROFILE_H
#define SOUND_PROFILE_H

#include <map>
#include <string>

#include <SDL_mixer.h>

class SoundProfile{
    std::string _id;

    std::map<std::string, Mix_Chunk *> _sounds;

public:
    SoundProfile(const std::string &id);

    bool operator<(const SoundProfile &rhs) const;

    void set(const std::string &type, std::string &filename);
    void play(const std::string &type) const;
};

#endif
