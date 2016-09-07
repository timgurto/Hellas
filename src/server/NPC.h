// (C) 2016 Tim Gurto

#ifndef NPC_H
#define NPC_H

#include "NPCType.h"
#include "Object.h"
#include "../Point.h"

// Objects that can engage in combat, and that are AI-driven
class NPC : public Object {
public:
    NPC(const NPCType *type, const Point &loc); // Generates a new serial

};

#endif
