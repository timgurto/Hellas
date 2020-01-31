#pragma once

#include "EntityComponent.h"
#include "ServerItem.h"
class Entity;

class Gatherable : EntityComponent {
 public:
  Gatherable(Entity &parent) : EntityComponent(parent) {}
  const ItemSet &gatherContents() const { return _gatherContents; }
  void gatherContents(const ItemSet &contents);
  // Randomly choose an item type for the user to gather.
  const ServerItem *chooseGatherItem() const;
  // Randomly choose a quantity of the above items, between 1 and the object's
  // contents.
  size_t chooseGatherQuantity(const ServerItem *item) const;
  void removeItem(const ServerItem *item,
                  size_t qty);  // From _gatherContents; gathering
  void populateGatherContents();

  void incrementGatheringUsers(const User *userToSkip = nullptr);
  void decrementGatheringUsers(const User *userToSkip = nullptr);
  void removeAllGatheringUsers();
  size_t numUsersGathering() const { return _numUsersGathering; }

 private:
  ItemSet _gatherContents;
  size_t _numUsersGathering{0};
};
