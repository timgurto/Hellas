// (C) 2016 Tim Gurto

#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include "ObjectType.h"

// Describes a class of NPCs, a la the ObjectType class.
class NPCType : public ObjectType{
    health_t _maxHealth;

public:
    NPCType(const std::string &id, health_t maxHealth);

    void maxHealth(health_t hp) { _maxHealth = hp; }
    health_t maxHealth() const { return _maxHealth; }

    virtual char classTag() const override { return 'n'; }

};

#endif
