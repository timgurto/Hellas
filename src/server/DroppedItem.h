#pragma once

#include "Entity.h"
#include "EntityType.h"

// Created when an item is dropped.  Allows that item to be gathered.
class DroppedItem : public Entity {
 public:
  class Type : public EntityType {
   public:
    Type();
    char classTag() const override { return 'i'; }
  };

  DroppedItem(const ServerItem &itemType, size_t quantity,
              const MapPoint &location);
  ~DroppedItem() {}

  char classTag() const override { return 'i'; }
  void sendInfoToClient(const User &targetUser) const override;
  ms_t timeToRemainAsCorpse() const override { return 0; }
  bool canBeAttackedBy(const User &) const override { return false; }
  void giveItemTo(User &user);

 private:
  static Type commonType;

  const ServerItem &_itemType{nullptr};
  size_t _quantity;
};
