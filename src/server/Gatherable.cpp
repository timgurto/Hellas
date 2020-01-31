#include "Gatherable.h"

#include "Server.h"

void Gatherable::incrementGatheringUsers(const User *userToSkip) {
  const auto &server = Server::instance();
  ++_numUsersGathering;
  if (_numUsersGathering == 1) {
    for (const User *user : server.findUsersInArea(_owner.location()))
      if (user != userToSkip)
        user->sendMessage({SV_GATHERING_OBJECT, _owner.serial()});
  }
}

void Gatherable::decrementGatheringUsers(const User *userToSkip) {
  const auto &server = Server::instance();
  --_numUsersGathering;
  if (_numUsersGathering == 0) {
    for (const User *user : server.findUsersInArea(_owner.location()))
      if (user != userToSkip)
        user->sendMessage({SV_NOT_GATHERING_OBJECT, _owner.serial()});
  }
}

void Gatherable::removeAllGatheringUsers() {
  const auto &server = Server::instance();
  _numUsersGathering = 0;
  for (const User *user : server.findUsersInArea(_owner.location()))
    user->sendMessage({SV_NOT_GATHERING_OBJECT, _owner.serial()});
}

void Gatherable::gatherContents(const ItemSet &contents) {
  _gatherContents = contents;
}

void Gatherable::removeItem(const ServerItem *item, size_t qty) {
  if (_gatherContents[item] < qty) {
    SERVER_ERROR("Attempting to remove contents when quantity is insufficient");
    qty = _gatherContents[item];
  }
  if (_gatherContents.totalQuantity() < qty) {
    SERVER_ERROR(
        "Attempting to remove contents when total quantity is insufficient");
  }
  _gatherContents.remove(item, qty);
}

void Gatherable::populateGatherContents() {
  if (!_owner.type()->yield) return;
  _owner.type()->yield.instantiate(_gatherContents);
}

const ServerItem *Gatherable::chooseGatherItem() const {
  if (_gatherContents.isEmpty()) {
    SERVER_ERROR("Can't gather from an empty object");
    return nullptr;
  }

  // Count number of average gathers remaining for each item type.
  size_t totalGathersRemaining = 0;
  std::map<const Item *, size_t> gathersRemaining;
  for (auto item : _gatherContents) {
    size_t qtyRemaining = item.second;
    double gatherSize =
        _owner.type()->yield.gatherMean(toServerItem(item.first));
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

size_t Gatherable::chooseGatherQuantity(const ServerItem *item) const {
  size_t randomQty = _owner.type()->yield.generateGatherQuantity(item);
  size_t qty = min<size_t>(randomQty, _gatherContents[item]);
  return qty;
}
