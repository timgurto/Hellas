#pragma once

#include "Entity.h"
#include "EntityType.h"

class Server;

// Created when an item is dropped.  Allows that item to be gathered.
class DroppedItem : public Entity {
 public:
  class Type : public EntityType {
   public:
    Type();
    char classTag() const override { return 'i'; }
  };

  DroppedItem(const ServerItem &itemType, Hitpoints health, size_t quantity,
              std::string suffix, const MapPoint &location);
  ~DroppedItem() {}

  char classTag() const override { return 'i'; }
  void sendInfoToClient(const User &targetUser,
                        bool isNew = false) const override;
  ms_t timeToRemainAsCorpse() const override { return 0; }
  bool canBeAttackedBy(const User &) const override { return false; }
  bool areOverlapsAllowedWith(const Entity &rhs) const override;
  void getPickedUpBy(User &user);
  Hitpoints itemHealth() const { return _itemHealth; }
  std::string suffix() const { return _suffix; }

  void writeToXML(XmlWriter &xw) const override;
  void addToItemCounts(std::map<std::string, int> &itemCounts) const;

  static Type TYPE;

 private:
  const ServerItem &_itemType{nullptr};
  size_t _quantity;
  Hitpoints _itemHealth;
  std::string _suffix;
};
