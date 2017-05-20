#ifndef NPC_H
#define NPC_H

#include "Entity.h"
#include "Loot.h"
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

    ms_t _corpseTime; // How long this combatant has been a corpse.
    State _state;


public:
    static const ms_t CORPSE_TIME; // How long dead combatants remain as corpse objects.
    static const size_t LOOT_CAPACITY; // The size of the container.

    NPC(const NPCType *type, const Point &loc); // Generates a new serial
    virtual ~NPC(){}

    const NPCType *npcType() const { return dynamic_cast<const NPCType *>(type()); }
    const Loot &loot() const { return _loot; }

    virtual health_t maxHealth() const override { return npcType()->maxHealth(); }
    virtual health_t attack() const override { return npcType()->attack(); }
    virtual ms_t attackTime() const override { return npcType()->attackTime(); }
    virtual double speed() const override { return 10; }
    virtual bool collides() const override;

    virtual void onHealthChange() override;
    virtual void onDeath() override;

    virtual char classTag() const override { return 'n'; }

    virtual void sendInfoToClient(const User &targetUser) const override;
    virtual void describeSelfToNewWatcher(const User &watcher) const override;
    virtual void alertWatcherOnInventoryChange(const User &watcher, size_t slot) const;

    virtual void writeToXML(XmlWriter &xw) const override;

    virtual void update(ms_t timeElapsed);
    void processAI(ms_t timeElapsed);

private:
    Loot _loot;
};

#endif
