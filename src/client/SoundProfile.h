#ifndef SOUND_PROFILE_H
#define SOUND_PROFILE_H

#include <map>
#include <string>

#include <SDL_mixer.h>

class SoundProfile{
    typedef int Channel;
    static const Channel NO_CHANNEL;

    std::string _id;

    std::map<std::string, Mix_Chunk *> _sounds;

    static std::vector<std::pair<std::string, const void *> > repeatingSounds;

public:
    SoundProfile(const std::string &id);

    bool operator<(const SoundProfile &rhs) const;

    void set(const std::string &type, std::string &filename);
    void play(const std::string &type) const;
    void playRepeated(const std::string &type, const void *source) const;
    void stopRepeated(const std::string &type, const void *source) const;
};

#endif
