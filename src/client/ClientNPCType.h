// (C) 2016 Tim Gurto

#ifndef CLIENT_NPC_TYPE_H
#define CLIENT_NPC_TYPE_H

#include "ClientObject.h"

class ClientNPCType : public ClientObjectType{
    health_t _maxHealth;

public:
    ClientNPCType(const std::string &id, health_t maxHealth);

    health_t maxHealth() const { return _maxHealth; }
    void maxHealth(health_t n) { _maxHealth = n; }

    virtual char classTag() const override { return 'n'; }
};

#endif
