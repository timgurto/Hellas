#ifndef NPC_H
#define NPC_H

#include "../Point.h"
#include "../SpellSchool.h"
#include "Entity.h"
#include "NPCType.h"
#include "ThreatTable.h"
#include "objects/Object.h"

class User;

// Objects that can engage in combat, and that are AI-driven
class NPC : public Entity, public QuestNode {
 public:
  enum Order { STAY, FOLLOW };

 private:
  enum State { IDLE, CHASE, ATTACK, PET_FOLLOW_OWNER };
  State _state;
  Order _order{FOLLOW};  // Indicates a desire; informs state changes in pets.
  Level _level{0};
  ThreatTable _threatTable;
  MapPoint _targetDestination{};
  ms_t _timeEngaged{0};  // For logging purposes

 public:
  NPC(const NPCType *type, const MapPoint &loc);  // Generates a new serial
  virtual ~NPC() {}

  const NPCType *npcType() const {
    return dynamic_cast<const NPCType *>(type());
  }

  ms_t timeToRemainAsCorpse() const override { return 600000; }  // 10 minutes
  bool canBeAttackedBy(const User &user) const override;
  bool canBeAttackedBy(const NPC &npc) const override;
  bool areOverlapsAllowedWith(const Entity &rhs) const;
  CombatResult generateHitAgainst(const Entity &target, CombatType type,
                                  SpellSchool school,
                                  px_t range) const override;
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

  void updateStats() override;
  void onHealthChange() override;
  void onDeath() override;
  void onAttackedBy(Entity &attacker, Threat threat) override;
  px_t attackRange() const override;
  void sendRangedHitMessageTo(const User &userToInform) const override;
  void sendRangedMissMessageTo(const User &userToInform) const override;
  virtual void broadcastDamagedMessage(Hitpoints amount) const override;
  virtual void broadcastHealedMessage(Hitpoints amount) const override;
  SpellSchool school() const override { return npcType()->school(); }
  int getLevelDifference(const User &user) const override;
  double combatDamage() const override {
    return npcType()->baseStats().physicalDamage;
  }
  bool grantsXPOnDeath() const override { return true; }
  double getTameChance() const;

  char classTag() const override { return 'n'; }

  void sendInfoToClient(const User &targetUser) const override;
  ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum,
                                                   const User &user) override;

  void writeToXML(XmlWriter &xw) const override;

  void update(ms_t timeElapsed);

  // AI
  static const px_t AGGRO_RANGE;
  static const px_t PURSUIT_RANGE;
  static const px_t FOLLOW_DISTANCE;
  static const px_t MAX_FOLLOW_RANGE;
  static const ms_t FREQUENCY_TO_LOOK_FOR_TARGETS;
  void order(Order newOrder) { _order = newOrder; }
  Order order() const { return _order; }
  // AI
 private:
  ms_t _timeSinceLookedForTargets;
  const User *_followTarget{nullptr};
  void processAI(ms_t timeElapsed);
  void getNewTargetsFromProximity(ms_t timeElapsed);
  void transitionIfNecessary();
  void onTransition(State previousState);
  void act();
  void setStateBasedOnOrder();
};

#endif
