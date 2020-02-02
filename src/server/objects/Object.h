#ifndef OBJECT_H
#define OBJECT_H

#include "../../Point.h"
#include "../Entity.h"
#include "../ItemSet.h"
#include "../Loot.h"
#include "../MerchantSlot.h"
#include "../QuestNode.h"
#include "../Transformation.h"
#include "Container.h"
#include "Deconstruction.h"
#include "ObjectType.h"

class User;
class XmlWriter;

// A server-side representation of an in-game object
class Object : public Entity, public QuestNode, public DamageOnUse {
  std::vector<MerchantSlot> _merchantSlots;

  ItemSet
      _remainingMaterials;  // The remaining construction costs, if relevant.

  ms_t _disappearTimer;  // When this hits zero, it disappears.

 public:
  Object(const ObjectType *type,
         const MapPoint &loc);  // Generates a new serial

  // Dummies.  TODO: Remove
  Object(size_t serial);
  Object(const MapPoint &loc);

  virtual ~Object();

  const ObjectType &objType() const {
    return *dynamic_cast<const ObjectType *>(type());
  }

  const std::vector<MerchantSlot> &merchantSlots() const {
    return _merchantSlots;
  }
  const MerchantSlot &merchantSlot(size_t slot) const {
    return _merchantSlots[slot];
  }
  MerchantSlot &merchantSlot(size_t slot) { return _merchantSlots[slot]; }
  bool isBeingBuilt() const {
    return !_remainingMaterials.isEmpty() && !isDead();
  }
  const ItemSet &remainingMaterials() const { return _remainingMaterials; }
  ItemSet &remainingMaterials() { return _remainingMaterials; }
  void clearMaterialsRequired() { _remainingMaterials.clear(); }

  bool hasContainer() const { return _container != nullptr; }
  Container &container() { return *_container; }
  const Container &container() const { return *_container; }

  bool hasDeconstruction() const { return _deconstruction.exists(); }
  Deconstruction &deconstruction() { return _deconstruction; }
  const Deconstruction &deconstruction() const { return _deconstruction; }

  bool isAbleToDeconstruct(const User &user) const;

  virtual char classTag() const override { return 'o'; }

  ms_t timeToRemainAsCorpse() const override { return 43200000; }  // 12 hours

  void writeToXML(XmlWriter &xw) const override;

  void update(ms_t timeElapsed) override;

  void onHealthChange() override;
  void onEnergyChange() override;
  void onDeath() override;
  bool canBeAttackedBy(const User &) const override;

  void setType(const ObjectType *type, bool skipConstruction = false,
               bool wasCalledFromConstructor = false);  // Set/change ObjectType

  void sendInfoToClient(const User &targetUser) const override;
  void tellRelevantUsersAboutInventorySlot(size_t slot) const;
  void tellRelevantUsersAboutMerchantSlot(size_t slot) const;
  ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum,
                                                   const User &user) override;
  Message outOfRangeMessage() const override;
  bool shouldAlwaysBeKnownToUser(const User &user) const override;
  virtual void broadcastDamagedMessage(Hitpoints amount) const override;
  virtual void broadcastHealedMessage(Hitpoints amount) const override;

  void populateLoot();

  friend class Container;  // TODO: Remove once everything is componentized

  // From DamageOnUse
  bool isBroken() const override { return isDead(); }
  void damageFromUse() override { reduceHealth(1); };

  void repair() override;

  Transformation transformation;

 private:
  Container *_container = nullptr;
  Deconstruction _deconstruction{};
};

#endif
