#pragma once

#include <map>

#include "Projectile.h"

struct ClientSpell {
    const Projectile::Type *projectile = nullptr;
    const SoundProfile *sounds = nullptr;
};

using ClientSpells = std::map<std::string, ClientSpell *>;
