#pragma once

#include <map>

#include "Projectile.h"

struct ClientSpell {
    const Projectile::Type *projectile = nullptr;
};

using ClientSpells = std::map<std::string, ClientSpell *>;
