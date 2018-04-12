#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include "LootTable.h"
#include "objects/ObjectType.h"

class ClientItem;

// Describes a class of NPCs, a la the ObjectType class.
class NPCType : public ObjectType{

    LootTable _lootTable;
    Level _level;
    bool _isRanged = false;
    SpellSchool _school{ SpellSchool::PHYSICAL };

public:
    static Stats BASE_STATS;

    NPCType(const std::string &id);
    virtual ~NPCType(){}

    static void init();
    
    const LootTable &lootTable() const { return _lootTable; }
    void level(Level l) { _level = l; }
    Level level() const { return _level; }
    void makeRanged() { _isRanged = true; }
    bool isRanged() const { return _isRanged; }
    void school(SpellSchool school) { _school = school; }
    SpellSchool school() const { return _school; }


    virtual char classTag() const override { return 'n'; }

    void addSimpleLoot(const ServerItem *item, double chance);
    void addNormalLoot(const ServerItem *item, double mean, double sd);

};

#endif
