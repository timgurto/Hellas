#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include "LootTable.h"
#include "ObjectType.h"

// Describes a class of NPCs, a la the ObjectType class.
class NPCType : public ObjectType{
    health_t _maxHealth;
    LootTable _lootTable;

public:
    NPCType(const std::string &id, health_t maxHealth);

    void maxHealth(health_t hp) { _maxHealth = hp; }
    health_t maxHealth() const { return _maxHealth; }
    const LootTable &lootTable() const { return _lootTable; }

    virtual char classTag() const override { return 'n'; }

    void addLoot(const ServerItem *item, double mean, double sd);

};

#endif
