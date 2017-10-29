#ifndef NPC_H
#define NPC_H

#include "Entity.h"
#include "NPCType.h"
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


public:
    NPC(const NPCType *type, const Point &loc); // Generates a new serial
    virtual ~NPC(){}

    const NPCType *npcType() const { return dynamic_cast<const NPCType *>(type()); }

    Hitpoints maxHealth() const override { return npcType()->maxHealth(); }
    Hitpoints attack() const override { return npcType()->attack(); }
    ms_t attackTime() const override { return npcType()->attackTime(); }
    double speed() const override { return 10; }
    ms_t timeToRemainAsCorpse() const override { return 600000; } // 10 minutes
    bool canBeAttackedBy(const User &user) const override { return true; }

    virtual void onHealthChange() override;
    virtual void onDeath() override;

    virtual char classTag() const override { return 'n'; }

    virtual void sendInfoToClient(const User &targetUser) const override;
    virtual void describeSelfToNewWatcher(const User &watcher) const override;
    virtual void alertWatcherOnInventoryChange(const User &watcher, size_t slot) const;
    virtual ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user) override;

    virtual void writeToXML(XmlWriter &xw) const override;

    virtual void update(ms_t timeElapsed);
    void processAI(ms_t timeElapsed);
};

#endif
