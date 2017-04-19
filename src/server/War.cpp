#include "War.h"

void Wars::declare(const Belligerent &a, const Belligerent &b){
    Belligerents sorted = sortAlphabetically(a, b);
    container.insert(sorted);
}

bool Wars::isAtWar(const Belligerent &a, const Belligerent &b) const{
    Belligerents sorted = sortAlphabetically(a, b);
    auto matches = container.equal_range(sorted.first);
    for (auto it = matches.first; it != matches.second; ++it){
        if (it->second == sorted.second)
            return true;
    }
    return false;
}

Wars::Belligerents Wars::sortAlphabetically(const Belligerent &a, const Belligerent &b){
    if (a < b)
        return std::make_pair(a, b);
    return std::make_pair(b, a);
}