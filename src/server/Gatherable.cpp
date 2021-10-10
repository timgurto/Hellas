#include "Gatherable.h"

#include "Server.h"

void Gatherable::incrementGatheringUsers(const User *userToSkip) {
  const auto &server = Server::instance();
  ++_numUsersGathering;
  if (_numUsersGathering == 1) {
    for (const User *user : server.findUsersInArea(parent().location()))
      if (user != userToSkip)
        user->sendMessage({SV_OBJECT_BEING_GATHERED, parent().serial()});
  }
}

void Gatherable::decrementGatheringUsers(const User *userToSkip) {
  const auto &server = Server::instance();
  --_numUsersGathering;
  if (_numUsersGathering == 0) {
    for (const User *user : server.findUsersInArea(parent().location()))
      if (user != userToSkip)
        user->sendMessage({SV_OBJECT_NOT_BEING_GATHERED, parent().serial()});
  }
}

void Gatherable::removeAllGatheringUsers() {
  const auto &server = Server::instance();
  _numUsersGathering = 0;
  for (const User *user : server.findUsersInArea(parent().location()))
    user->sendMessage({SV_OBJECT_NOT_BEING_GATHERED, parent().serial()});
}

void Gatherable::setContents(const ItemSet &contents) { _contents = contents; }

void Gatherable::removeItem(const ServerItem *item, size_t qty) {
  if (_contents[item] < qty) {
    SERVER_ERROR("Attempting to remove contents when quantity is insufficient");
    qty = _contents[item];
  }
  if (_contents.totalQuantity() < qty) {
    SERVER_ERROR(
        "Attempting to remove contents when total quantity is insufficient");
  }
  _contents.remove(item, qty);
}

void Gatherable::populateContents() {
  if (!parent().type()->yield) return;
  parent().type()->yield.instantiate(_contents);
}

void Gatherable::clearContents() { _contents.clear(); }

const ServerItem *Gatherable::chooseRandomItem() const {
  if (_contents.isEmpty()) {
    SERVER_ERROR("Can't gather from an empty object");
    return nullptr;
  }

  // Count number of average gathers remaining for each item type.
  size_t totalGathersRemaining = 0;
  std::map<const Item *, size_t> gathersRemaining;
  for (auto item : _contents) {
    size_t qtyRemaining = item.second;
    double gatherSize =
        parent().type()->yield.gatherMean(toServerItem(item.first));
    size_t remaining = static_cast<size_t>(ceil(qtyRemaining / gatherSize));
    gathersRemaining[item.first] = remaining;
    totalGathersRemaining += remaining;
  }

  if (totalGathersRemaining == 0) {
    SERVER_ERROR("Invalid gather count");
    return nullptr;
  }

  // Choose random item, weighted by remaining gathers.
  size_t i = rand() % totalGathersRemaining;
  for (auto item : gathersRemaining) {
    if (i <= item.second)
      return toServerItem(item.first);
    else
      i -= item.second;
  }
  SERVER_ERROR("No item was found to gather");
  return nullptr;
}

size_t Gatherable::chooseRandomQuantity(const ServerItem *item) const {
  size_t randomQty = parent().type()->yield.generateGatherQuantity(item);
  size_t qty = min<size_t>(randomQty, _contents[item]);
  return qty;
}
