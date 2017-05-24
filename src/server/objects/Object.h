#ifndef OBJECT_H
#define OBJECT_H

#include "Container.h"
#include "Deconstruction.h"
#include "ObjectType.h"
#include "../Entity.h"
#include "../MerchantSlot.h"
#include "../ItemSet.h"
#include "../Permissions.h"
#include "../../Point.h"

class User;
class XmlWriter;

// A server-side representation of an in-game object
class Object : public Entity{
    Permissions _permissions;
    ItemSet _contents; // Remaining contents, which can be gathered
    std::vector<MerchantSlot> _merchantSlots;

    size_t _numUsersGathering; // The number of users gathering from this object.

    ItemSet _remainingMaterials; // The remaining construction costs, if relevant.

    ms_t _transformTimer; // When this hits zero, it switches types.

public:
    Object(const ObjectType *type, const Point &loc); // Generates a new serial

    // Dummies.  TODO: Remove
    Object(size_t serial);
    Object(const Point &loc);

    virtual ~Object();

    const ObjectType &objType() const { return * dynamic_cast<const ObjectType *>(type()); }

    const ItemSet &contents() const { return _contents; }
    void contents(const ItemSet &contents);
    const std::vector<MerchantSlot> &merchantSlots() const { return _merchantSlots; }
    const MerchantSlot &merchantSlot(size_t slot) const { return _merchantSlots[slot]; }
    MerchantSlot &merchantSlot(size_t slot) { return _merchantSlots[slot]; }
    void incrementGatheringUsers(const User *userToSkip = nullptr);
    void decrementGatheringUsers(const User *userToSkip = nullptr);
    void removeAllGatheringUsers();
    size_t numUsersGathering() const { return _numUsersGathering; }
    bool isBeingBuilt() const { return !_remainingMaterials.isEmpty(); }
    const ItemSet &remainingMaterials() const { return _remainingMaterials; }
    ItemSet &remainingMaterials() { return _remainingMaterials; }
    void removeMaterial(const ServerItem *item, size_t qty);
    bool isTransforming() const { return _transformTimer > 0; }
    ms_t transformTimer() const { return _transformTimer; }
    Permissions &permissions() { return _permissions; }
    const Permissions &permissions() const { return _permissions; }

    bool hasContainer() const { return _container != nullptr; }
    Container &container() { return *_container; }
    const Container &container() const { return *_container; }

    bool hasDeconstruction() const { return _deconstruction != nullptr; }
    Deconstruction &deconstruction() { return *_deconstruction; }
    const Deconstruction &deconstruction() const { return *_deconstruction; }

    bool isAbleToDeconstruct(const User &user) const;

    virtual char classTag() const override { return 'o'; }

    virtual health_t maxHealth() const override { return objType().strength(); }
    virtual health_t attack() const override { return 0; }
    virtual ms_t attackTime() const override { return 0; }
    virtual ms_t timeToRemainAsCorpse() const override { return 5000/*43200000*/; } // 12 hours

    virtual void writeToXML(XmlWriter &xw) const override;

    virtual void update(ms_t timeElapsed) override;

    virtual void onHealthChange() override;
    virtual void onDeath() override;

    void setType(const ObjectType *type); // Set/change ObjectType

    virtual void sendInfoToClient(const User &targetUser) const override;
    virtual void describeSelfToNewWatcher(const User &watcher) const override;
    virtual void alertWatcherOnInventoryChange(const User &watcher, size_t slot) const;
    virtual ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user) override;

    // Randomly choose an item type for the user to gather.
    const ServerItem *chooseGatherItem() const;
    // Randomly choose a quantity of the above items, between 1 and the object's contents.
    size_t chooseGatherQuantity(const ServerItem *item) const;
    void removeItem(const ServerItem *item, size_t qty); // From _contents; gathering

    friend class Container; // TODO: Remove once everything is componentized

private:
    Container *_container;
    Deconstruction *_deconstruction;
};

#endif
