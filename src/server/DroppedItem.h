#pragma once

#include "Entity.h"
#include "EntityType.h"

// Created when an item is dropped.  Allows that item to be gathered.
class DroppedItem : public Entity {
 public:
  class Type : public EntityType {
   public:
    Type() : EntityType("droppedItem") {}
    char classTag() const override { return 'i'; }
  };

  DroppedItem() : Entity(&commonType, MapPoint({})) {}

  char classTag() const override { return 'i'; }
  void sendInfoToClient(const User &targetUser) const override {}
  ms_t timeToRemainAsCorpse() const override { return 0; }
  bool canBeAttackedBy(const User &) const override { return false; }

 private:
  static Type commonType;
};
