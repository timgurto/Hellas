#ifndef SOUND_PROFILE_H
#define SOUND_PROFILE_H

#include <map>
#include <string>
#include <vector>

#include <SDL_mixer.h>

class SoundsRecord;

typedef std::string SoundType;
typedef int Channel;
const Channel NO_CHANNEL = -1;

class SoundProfile{
    std::string _id;

    std::map<SoundType, Mix_Chunk *> _sounds;

    static SoundsRecord loopingSounds;

    Channel checkAndPlaySound(const SoundType &type, bool loop) const;

public:
    SoundProfile(const std::string &id);

    bool operator<(const SoundProfile &rhs) const;

    void set(const SoundType &type, std::string &filename);
    void playOnce(const SoundType &type) const;
    void startLooping(const SoundType &type, const void *source) const;
    void stopLooping(const SoundType &type, const void *source) const;
};



// A mapping of channel -> sound source/type
class SoundsRecord{
    struct Entry{
        SoundType type;
        const void *source;
    public:
        Entry(const SoundType &type, const void *source): type(type), source(source) {}
        static const Entry BLANK;
    };

    std::vector<Entry> _record;

public:
    SoundsRecord();
    void set(const SoundType &type, const void *source, Channel channel);
    void unset(Channel channel);
    Channel SoundsRecord::getChannel(const std::string &type, const void *source) const;
};

#endif
