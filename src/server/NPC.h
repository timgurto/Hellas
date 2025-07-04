#ifndef NPC_H
#define NPC_H

#include "../Point.h"
#include "../SpellSchool.h"
#include "AI.h"
#include "Entity.h"
#include "NPCType.h"
#include "ThreatTable.h"
#include "objects/Object.h"

class User;

// Objects that can engage in combat, and that are AI-driven
class NPC : public Entity, public QuestNode {
 public:
 private:
  Level _level{0};
  ThreatTable _threatTable;
  ms_t _timeEngaged{0};  // For logging purposes

  ms_t _disappearTimer;  // When this hits zero, it disappears.

 public:
  NPC(const NPCType *type, const MapPoint &loc);  // Generates a new serial
  virtual ~NPC() {}

  const NPCType *npcType() const {
    return dynamic_cast<const NPCType *>(type());
  }

  AI ai;

  ms_t timeToRemainAsCorpse() const override { return 600000; }  // 10 minutes
  bool shouldBeIgnoredByAIProximityAggro() const override;
  bool canBeAttackedBy(const User &user) const override;
  bool canBeAttackedBy(const NPC &npc) const override;
  bool canBeDamagedAndWarn() const override;
  bool canAttack(const Entity &other) const override;
  bool areOverlapsAllowedWith(const Entity &rhs) const;
  bool canBeHealedBySpell() const override { return true; }
  void scaleThreatAgainst(Entity &target, double multiplier) override;
  void makeAwareOf(Entity &entity);
  bool isAwareOf(Entity &entity) const;
  void forgetAbout(const Entity &entity);
  void makeNearbyNPCsAwareOf(Entity &entity);
  void addThreat(User &attacker, Threat amount);
  Level level() const override { return _level; }
  Message outOfRangeMessage() const override;

  Permissions::Owner owner() const { return permissions.owner(); }
  virtual void onOwnershipChange() override;
  const User *followTarget() const { return _followTarget; }

  void updateStats() override;
  void onHealthChange() override;
  void restoreHealthAndBroadcastTo(const MapPoint &p);
  void broadcastHealthTo(const MapPoint &p);
  void onDeath() override;
  void onAttackedBy(Entity &attacker, Threat threat,
                    CombatResult result) override;
  px_t attackRange() const override;
  void sendRangedHitMessageTo(const User &userToInform) const override;
  void sendRangedMissMessageTo(const User &userToInform) const override;
  virtual void broadcastDamagedMessage(Hitpoints amount) const override;
  virtual void broadcastHealedMessage(Hitpoints amount) const override;
  SpellSchool school() const override { return npcType()->school(); }
  int getLevelDifference(const User &user) const override;
  double combatDamage() const override;
  bool grantsXPOnDeath() const override { return type()->rewardsXP; }
  virtual bool canHaveCustomName() const override { return true; }
  double getTameChance() const;

  char classTag() const override { return 'n'; }

  void sendInfoToClient(const User &targetUser,
                        bool isNew = false) const override;
  ServerItem::Instance *getSlotToTakeFromAndSendErrors(
      size_t slotNum, const User &user) override;

  void writeToXML(XmlWriter &xw) const override;

  void update(ms_t timeElapsed);

  // AI
 private:
  ms_t _timeSinceLookedForTargets;
  const User *_followTarget{nullptr};
  void getNewTargetsFromProximity(ms_t timeElapsed);

  friend class AI;
};

#endif
