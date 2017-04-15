#ifndef SOUND_PROFILE_H
#define SOUND_PROFILE_H

#include <map>
#include <set>

#include "types.h"

class SoundProfile{
    std::set<std::string>
        types,
        missingTypes;

public:
    void addType(const std::string &soundType) { types.insert(soundType); }
    void checkType(const std::string &soundType){
        if (types.count(soundType) == 0)
            missingTypes.insert(soundType);
    }
    const std::set<std::string> &getMissingTypes() const { return missingTypes; }
};

#endif
