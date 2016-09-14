// (C) 2016 Tim Gurto

#ifndef NPC_H
#define NPC_H

#include "Combatant.h"
#include "NPCType.h"
#include "Object.h"
#include "../Point.h"

// Objects that can engage in combat, and that are AI-driven
class NPC : public Object, public Combatant {
public:
    NPC(const NPCType *type, const Point &loc); // Generates a new serial

    virtual health_t maxHealth() const override { return npcType()->maxHealth(); }
    const NPCType *npcType() const { return dynamic_cast<const NPCType *>(type()); }

    virtual char classTag() const override { return 'n'; }
};

#endif
