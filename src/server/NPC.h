#ifndef NPC_H
#define NPC_H

#include "Entity.h"
#include "NPCType.h"
#include "ThreatTable.h"
#include "objects/Object.h"
#include "../Point.h"

// Objects that can engage in combat, and that are AI-driven
class NPC : public Entity {
    enum State {
        IDLE,
        CHASE,
        ATTACK,
    };
    State _state;
    Level _level{ 0 };
    ThreatTable _threatTable;

public:
    NPC(const NPCType *type, const MapPoint &loc); // Generates a new serial
    virtual ~NPC(){}

    const NPCType *npcType() const { return dynamic_cast<const NPCType *>(type()); }

    ms_t timeToRemainAsCorpse() const override { return 600000; } // 10 minutes
    bool canBeAttackedBy(const User &user) const override { return true; }
    CombatResult generateHitAgainst(const Entity &target, CombatType type, SpellSchool school, px_t range) const override;
    void scaleThreatAgainst(Entity &target, double multiplier) override;

    void updateStats() override;
    void onHealthChange() override;
    void onDeath() override;
    void onAttackedBy(Entity &attacker, Hitpoints damage) override;
    px_t attackRange() const override;
    void sendRangedHitMessageTo(const User &userToInform) const override;
    void sendRangedMissMessageTo(const User &userToInform) const override;

    char classTag() const override { return 'n'; }

    void sendInfoToClient(const User &targetUser) const override;
    void describeSelfToNewWatcher(const User &watcher) const override;
    void alertWatcherOnInventoryChange(const User &watcher, size_t slot) const;
    ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user) override;

    void writeToXML(XmlWriter &xw) const override;

    void update(ms_t timeElapsed);
    void processAI(ms_t timeElapsed);
    void forgetAbout(const Entity &entity);
};

#endif
