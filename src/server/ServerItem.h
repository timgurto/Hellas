#ifndef SERVER_ITEM_H
#define SERVER_ITEM_H

#include <SDL.h>

#include <cassert>
#include <map>

#include "../Item.h"
#include "../Serial.h"
#include "DamageOnUse.h"
#include "ItemSet.h"

class ObjectType;
class User;
class ItemSet;

// Describes an item type
class ServerItem : public Item {
 public:
  class Instance : public DamageOnUse {
    // Assumption: any item type that can have a meaningful state, cannot stack.

   public:
    class ReportingInfo {  // Used to report changes
     public:
      ReportingInfo() {}
      static ReportingInfo InObjectContainer() { return {}; }
      static ReportingInfo DummyUser() { return {}; }
      static ReportingInfo UserGear(const User *owner, size_t slot);
      static ReportingInfo UserInventory(const User *owner, size_t slot);

      void report();

     private:
      ReportingInfo(const User *owner, Serial container, size_t slot)
          : _owner(owner), _container(container), _slot(slot) {}
      const User *_owner{nullptr};  // If null, no reporting necessary
      Serial _container;            // Should be either INVENTORY or GEAR
      size_t _slot{0};
    };

    Instance() = default;
    Instance(const ServerItem *type, ReportingInfo info);
    static Instance LoadFromFile(const ServerItem *type, ReportingInfo info,
                                 Hitpoints health) {
      return {type, info, health};
    }

    static void swap(std::pair<ServerItem::Instance, size_t> &lhs,
                     std::pair<ServerItem::Instance, size_t> &rhs);

    bool hasItem() const { return _type != nullptr; }
    const ServerItem *type() const { return _type; }
    Hitpoints health() const { return _health; }
    void initHealth(Hitpoints startingHealth) { _health = startingHealth; }
    bool isBroken() const;
    void damageFromUse() override;
    void damageOnPlayerDeath() override;
    void repair() override;
    double toolSpeed(const std::string &tag) const override;
    void onEquip() { _hasBeenEquipped = true; }
    bool isSoulbound() const;
    StatsMod statsFromSuffix() const { return _statsFromSuffix; }

   private:
    Instance(const ServerItem *type, ReportingInfo info, Hitpoints health);
    const ServerItem *_type{nullptr};
    Hitpoints _health{0};
    bool _hasBeenEquipped{false};
    ReportingInfo _reportingInfo;
    StatsMod _statsFromSuffix{};
  };

  enum ContainerCheckResult {
    ITEMS_PRESENT,
    ITEMS_MISSING,
    ITEMS_SOULBOUND,
    ITEMS_BROKEN
  };

 private:
  size_t _stackSize{1};

  // The object that this item can construct
  const ObjectType *_constructsObject{nullptr};

  // An item returned to the user after this is used as a construction material
  const ServerItem *_returnsOnConstruction{nullptr};

  // An item returned to the user after a spell is cast from this
  const ServerItem *_returnsOnCast{nullptr};
  bool _isLostOnCast{true};

  bool _loaded{false};

  std::string _exclusiveToQuest{};

 public:
  ServerItem(const std::string &id);

  typedef std::pair<ServerItem::Instance, size_t> Slot;
  typedef std::vector<Slot> vect_t;

  size_t stackSize() const { return _stackSize; }
  void stackSize(size_t n) { _stackSize = n; }
  void constructsObject(const ObjectType *obj) { _constructsObject = obj; }
  const ObjectType *constructsObject() const { return _constructsObject; }
  const ServerItem *returnsOnConstruction() const {
    return _returnsOnConstruction;
  }
  void returnsOnConstruction(const ServerItem *item) {
    _returnsOnConstruction = item;
  }
  const ServerItem *returnsOnCast() const { return _returnsOnCast; }
  void returnsOnCast(const ServerItem *item) { _returnsOnCast = item; }
  void keepOnCast() { _isLostOnCast = false; }
  bool isLostOnCast() const { return _isLostOnCast; }
  void exclusiveToQuest(const std::string &id) { _exclusiveToQuest = id; }
  bool isQuestExclusive() const { return !_exclusiveToQuest.empty(); }
  const std::string &exclusiveToQuest() const { return _exclusiveToQuest; }
  bool valid() const { return _loaded; }
  void loaded() { _loaded = true; }
  bool canBeDamaged() const;
  bool isGear() const;

  void fetchAmmoItem() const override;
};

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item,
                  size_t qty = 1);
bool vectHasSpace(ServerItem::vect_t vect, ItemSet items);
bool vectHasSpaceAfterRemovingItems(const ServerItem::vect_t &vect,
                                    const ServerItem *item, size_t qty,
                                    const ServerItem *itemThatWillBeRemoved,
                                    size_t qtyThatWillBeRemoved);

ServerItem::ContainerCheckResult containerHasEnoughToTrade(
    const ServerItem::vect_t &container, const ItemSet &items);

const ServerItem *toServerItem(const Item *item);

#endif
