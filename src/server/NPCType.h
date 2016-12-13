#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include "LootTable.h"
#include "ObjectType.h"

// Describes a class of NPCs, a la the ObjectType class.
class NPCType : public ObjectType{
    health_t _maxHealth;
    health_t _attack;
    ms_t _attackTime;

    LootTable _lootTable;

public:
    NPCType(const std::string &id, health_t maxHealth);
    virtual ~NPCType(){}
    
    void maxHealth(health_t hp) { _maxHealth = hp; }
    health_t maxHealth() const { return _maxHealth; }
    health_t attack() const { return _attack; }
    void attack(health_t damage) { _attack = damage; }
    ms_t attackTime() const { return _attackTime; }
    void attackTime(ms_t time) { _attackTime = time; }
    const LootTable &lootTable() const { return _lootTable; }

    virtual char classTag() const override { return 'n'; }

    void addLoot(const ServerItem *item, double mean, double sd);

};

#endif
