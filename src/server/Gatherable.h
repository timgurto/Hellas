#pragma once

#include "EntityComponent.h"
#include "ServerItem.h"
class Entity;

class Gatherable : EntityComponent {
 public:
  Gatherable(Entity &parent) : EntityComponent(parent) {}

  const ItemSet &contents() const { return _contents; }
  void setContents(const ItemSet &contents);
  void populateContents();
  bool hasItems() const { return !_contents.isEmpty(); }
  void clearContents();

  // Randomly choose an item type for the user to gather.
  const ServerItem *chooseRandomItem() const;
  // Random quantity of the above item, between 1 and the object's contents.
  size_t chooseRandomQuantity(const ServerItem *item) const;
  void removeItem(const ServerItem *item, size_t qty);

  void incrementGatheringUsers(const User *userToSkip = nullptr);
  void decrementGatheringUsers(const User *userToSkip = nullptr);
  void removeAllGatheringUsers();
  size_t numUsersGathering() const { return _numUsersGathering; }

 private:
  ItemSet _contents;
  size_t _numUsersGathering{0};
};
