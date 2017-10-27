#pragma once

#include <map>

#include "../types.h"

struct Spell {
    health_t damage = 0;
};

using Spells = std::map<std::string, Spell *>;
