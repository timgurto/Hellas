// (C) 2016 Tim Gurto

#include "NPCType.h"

NPCType::NPCType(const std::string &id, health_t maxHealth):
ObjectType(id),
_maxHealth(maxHealth)
{}
