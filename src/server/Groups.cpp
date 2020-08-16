#include "Groups.h"

#include <cassert>

void Groups::createGroup(User& founder) {
  auto newGroup = Group{};
  newGroup.insert(&founder);
  _mutex.lock();
  _groups.push_back(newGroup);
  _mutex.unlock();
}

void Groups::addToGroup(User& newMember, User& leader) {
  _mutex.lock();
  for (auto& group : _groups) {
    if (group.find(&leader) != group.end()) {
      group.insert(&newMember);
      _mutex.unlock();
      return;
    }
  }
  _mutex.unlock();
  assert(false);
}

Groups::Group Groups::getMembersOfPlayersGroup(User& player) const {
  _mutex.lock();
  for (const auto& group : _groups)
    if (group.find(&player) != group.end()) {
      _mutex.unlock();
      return group;
    }
  _mutex.unlock();

  auto soloResult = Group{};
  soloResult.insert(&player);
  return soloResult;
}

int Groups::getGroupSize(const User& u) const {
  _mutex.lock();
  for (const auto& group : _groups) {
    auto* nonConstUser = const_cast<User*>(&u);
    if (group.find(nonConstUser) != group.end()) {
      _mutex.unlock();
      return group.size();
    }
  }
  _mutex.unlock();
  return 1;
}

bool Groups::isUserInAGroup(const User& u) const {
  _mutex.lock();
  for (const auto& group : _groups) {
    auto* nonConstUser = const_cast<User*>(&u);
    if (group.find(nonConstUser) != group.end()) {
      _mutex.unlock();
      return true;
    }
  }
  _mutex.unlock();
  return false;
}
