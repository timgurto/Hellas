// (C) 2016 Tim Gurto

#include "ClientNPCType.h"

ClientNPCType::ClientNPCType(const std::string &id, health_t maxHealth):
ClientObjectType(id),
_maxHealth(maxHealth)
{}
