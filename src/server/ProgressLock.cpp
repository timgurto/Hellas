#include "ProgressLock.h"

#include <set>

#include "Server.h"

ProgressLock::locksByType_t ProgressLock::locksByType;
std::set<ProgressLock> ProgressLock::stagedLocks;

ProgressLock::ProgressLock(Type triggerType, const std::string &triggerID,
                           Type effectType, const std::string &effectID,
                           double chance)
    : _triggerType(triggerType),
      _triggerID(triggerID),
      _effectType(effectType),
      _effectID(effectID),
      _chance(chance) {}

void ProgressLock::registerStagedLocks() {
  for (ProgressLock lock : stagedLocks) {
    const Server &server = Server::instance();
    switch (lock._triggerType) {
      case ITEM:
      case GATHER: {
        auto it = server._items.find(lock._triggerID);
        if (it != server._items.end()) lock._trigger = &*it;
        break;
      }
      case CONSTRUCTION:
        lock._trigger = server.findObjectTypeByID(lock._triggerID);
        break;
      case RECIPE: {
        auto it = server._recipes.find(lock._triggerID);
        if (it != server._recipes.end()) lock._trigger = &*it;
        break;
      }
      default:;
    }

    if (lock._trigger == nullptr) {
      server._debug << Color::CHAT_ERROR << "Invalid progress trigger: '"
                    << lock._triggerID << "'" << Log::endl;
      continue;
    }

    switch (lock._effectType) {
      case RECIPE: {
        auto it = server._recipes.find(lock._effectID);
        if (it != server._recipes.end()) lock._effect = &*it;
        break;
      }
      case CONSTRUCTION:
        lock._effect = server.findObjectTypeByID(lock._effectID);
        break;
      default:;
    }

    if (lock._effect == nullptr) {
      server._debug << Color::CHAT_ERROR << "Invalid progress effect: '"
                    << lock._effectID << "'" << Log::endl;
      continue;
    }

    locksByType[lock._triggerType].insert(std::make_pair(lock._trigger, lock));
  }
}

void ProgressLock::unlockAll(User &user) {
  std::set<ProgressLock> allLocks;
  for (const auto &pair : locksByType[RECIPE]) allLocks.insert(pair.second);
  for (const auto &pair : locksByType[CONSTRUCTION])
    allLocks.insert(pair.second);
  for (const auto &pair : locksByType[ITEM]) allLocks.insert(pair.second);
  for (const auto &pair : locksByType[GATHER]) allLocks.insert(pair.second);

  std::set<std::string> newRecipes, newBuilds;
  for (const ProgressLock &lock : allLocks) {
    const std::string &id = lock._effectID;
    switch (lock._effectType) {
      case RECIPE:
        newRecipes.insert(id);
        user.addRecipe(id);
        break;
      case CONSTRUCTION:
        newBuilds.insert(id);
        user.addConstruction(id);
        break;
    }
  }
  const Server &server = Server::instance();
  server.sendNewBuildsMessage(user, newBuilds);
  server.sendNewRecipesMessage(user, newRecipes);
}

void ProgressLock::triggerUnlocks(User &user, Type triggerType,
                                  const void *trigger) {
  const locks_t &locks = locksByType[triggerType];
  auto toUnlock = locks.equal_range(trigger);

  std::set<std::string> newRecipes, newBuilds;

  for (auto it = toUnlock.first; it != toUnlock.second; ++it) {
    const ProgressLock &lock = it->second;

    auto roll = randDouble();
    auto unlockChance = user.stats().bonusUnlockChance.addTo(lock._chance);
    bool shouldUnlock = roll <= unlockChance;
    if (!shouldUnlock) continue;

    std::string id;
    switch (lock._effectType) {
      case RECIPE: {
        const auto *recipe = reinterpret_cast<const SRecipe *>(lock._effect);
        if (!recipe) break;
        id = recipe->id();
        if (user.knowsRecipe(id)) continue;
        newRecipes.insert(id);
        user.addRecipe(id);
        break;
      }
      case CONSTRUCTION: {
        const auto *objType =
            reinterpret_cast<const ObjectType *>(lock._effect);
        if (!objType) break;
        id = objType->id();
        if (user.knowsConstruction(id)) continue;
        newBuilds.insert(id);
        user.addConstruction(id);
        break;
      }
      default:;
    }
  }

  const Server &server = Server::instance();
  server.sendNewBuildsMessage(user, newBuilds);
  server.sendNewRecipesMessage(user, newRecipes);
}

void ProgressLock::cleanup() {
  locksByType.clear();
  stagedLocks.clear();
}

// Order doesnt' really matter, as long as it's a proper ordering.
bool ProgressLock::operator<(const ProgressLock &rhs) const {
  if (_triggerType != rhs._triggerType) return _triggerType < rhs._triggerType;
  if (_effectType != rhs._effectType) return _effectType < rhs._effectType;
  if (_triggerID != rhs._triggerID) return _triggerID < rhs._triggerID;
  return _effectID < rhs._effectID;
}
